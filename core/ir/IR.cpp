#include <core/ir/IR.hpp>
#include <core/Exception.hpp>

namespace blast::core::ir {

namespace {

Opcode opcodeFor(const std::string& op) {
    if (op == "+") return Opcode::ADD;
    if (op == "*") return Opcode::MUL;
    if (op == ">") return Opcode::GT;
    throw CodegenError("unsupported operator '" + op + "'");
}

} // namespace





void SSAIR::setLKO(context::Symbol* s, const Operand& o) {
    if (s) {
        this->m_lko.insert_or_assign(
            std::make_pair(s->id(), this->currentBlock().id()), o
        );
    }
}

// The binding for 's' as seen from 'bid': the one recorded there, else the
// one its predecessors carry, else a phi picking between them.
Operand SSAIR::getLKO(context::Symbol* s, BlockId bid) {
    if (s == nullptr) {
        return NONE();
    }

    // Can we find a reference of this symbol in block 'bid' ?
    // If so, return it
    auto it = this->m_lko.find(std::make_pair(s->id(), bid));
    if (it != this->m_lko.end()) {
        return it->second;
    }

    const std::vector<BlockId> preds = this->currentFct().getBlock(bid).preds();
    // No preds (entry block for example)
    if (preds.empty()) {
        return NONE();
    }

    // If bid only has on predecessor, check upstream
    if (preds.size() == 1) {
        return this->getLKO(s, preds[0]);
    }

    // If there's more predecessors, we are in a 'phi' situation
    // Add this phi to bid, typed after the first incoming
    const Operand phi = this->currentFct().addPhi(bid, this->getLKO(s, preds[0]).m_type);
    const std::size_t idx = this->currentFct().getBlock(bid).phis().size() - 1;

    // The next time getLKO is called on the same pair (s, bid), the phi is already computed and we can return immediately
    this->m_lko.insert_or_assign(
        std::make_pair(s->id(), bid), phi
    );

    std::vector<std::pair<BlockId, Operand>> incommings;
    for(const auto& pred: preds) {
        incommings.push_back(std::make_pair(pred, getLKO(s, pred)));
    }
    // Set the incommings
    this->currentFct().getBlock(bid).phis()[idx].m_incomings = std::move(incommings);

    // TODO: If all incommings are actually the same operand, we can skip the 'phi'

    return phi;
}




Operand SSAIR::visitIntLiteral(const parser::IntLiteral& node) {
    return LITERAL(node.value());
}

// Operand's LITERAL payload is an int64, so there is nowhere to put these yet.
// Fail loudly rather than lower them to something they are not: a silent
// visitEmpty() here is indistinguishable from a correctly empty block.
Operand SSAIR::visitFloatLiteral(const parser::FloatLiteral&) {
    throw CodegenError("float literals are not lowered yet");
}

Operand SSAIR::visitBoolLiteral(const parser::BoolLiteral&) {
    throw CodegenError("bool literals are not lowered yet");
}

Operand SSAIR::visitStringLiteral(const parser::StringLiteral&) {
    throw CodegenError("string literals are not lowered yet");
}

Operand SSAIR::visitIdentifier(const parser::Identifier& node) {
    // Get the associated symbols's register (if it exist)
    return this->getLKO(this->m_ctx->getNodeSymbol(&node), this->m_current_block);
}

Operand SSAIR::visitBinaryExpr(const parser::BinaryExpr& node) {
    const Operand lhs = this->visit(node.lhs());
    const Operand rhs = this->visit(node.rhs());
    return this->addInstruction(lhs, rhs, opcodeFor(node.op()));
}

Operand SSAIR::visitVarDecl(const parser::VarDecl& node) {
    Operand reg;
    context::Symbol* v = this->m_ctx->getNodeSymbol(&node);
    if (node.hasInit()) {
        reg = this->visit(node.init());
    } else {
        // Get default assign (0 for int)
        reg = LITERAL(std::int64_t{0});
    }
    // Register attributer register for variable
    this->setLKO(v, reg);
    return reg;
}

Operand SSAIR::visitAssign(const parser::Assign& node) {
    const Operand rhs = this->visit(node.value());
    context::Symbol* s_lhs = this->m_ctx->getNodeSymbol(node.target());
    this->setLKO(s_lhs, rhs);
    return rhs;
}

Operand SSAIR::visitExprStmt(const parser::ExprStmt& node) {
    return this->visit(node.expr());
}


Operand SSAIR::visitIfStmt(const parser::IfStmt& node) {
    Operand cond = this->visit(node.cond());
    const BlockId cond_id = this->m_current_block;

    // Create the 'then' block as predecessor of the 'cond' block
    const BlockId then_id = this->currentFct().addBlock("if.then");
    // Creat the 'join' block, that is reach after 'then' or if 'cond' is false
    const BlockId join_id = this->currentFct().addBlock("if.join");

    // Add 'then' and 'join' as successors of 'cond'
    this->currentFct().getBlock(then_id).preds().push_back(cond_id);
    this->currentFct().getBlock(join_id).preds().push_back(cond_id);
    // if a goto 'then' else goto 'join'
    this->currentFct().addCBR(cond_id, cond, then_id, join_id);

    // Move to the new 'then' block to visit it
    this->m_current_block = then_id;
    this->visit(node.thenBranch());

    // A nested 'if' leaves us in its own join, so branch from where we landed.
    const BlockId then_end = this->m_current_block;
    this->currentFct().getBlock(join_id).preds().push_back(then_end);
    // Unconditional jump from 'then' to 'join'
    this->currentFct().addBR(then_end, join_id);
    // Continue forward after 'join' block
    this->m_current_block = join_id;
    return NONE();
}

Operand SSAIR::visitBlock(const parser::Block& node) {
    Operand last = NONE();
    for(const auto& stmt: node.stmts()) {
        last = this->visit(stmt.get());
    }
    return last;
}


Operand SSAIR::visitTranslationUnit(const parser::TranslationUnit& node) {
    Operand last = NONE();
    for (const auto& stmt: node.stmts()){
        last = this->visit(stmt.get());
    }
    return last;
}

const Module& SSAIR::run(const parser::TranslationUnit& unit) {
    const Operand last = this->visit(&unit);
    // Return 0
    this->addInstruction(LITERAL(std::int64_t{0}), NONE(), Opcode::RET);
    return this->m_main;
}


std::vector<BlockId> BasicBlock::successors() const {
    std::vector<BlockId> scs;
    if (this->instrs().empty()) {
        return scs;
    }

    scs.reserve(2);
    const Instruction& instr = this->instrs().back();

    switch (instr.op()) {
        case Opcode::RET: {
            break;
        }
        case Opcode::BR: {
            scs.push_back(instr.lhs().m_block);
            break;
        }
        case Opcode::CBR: {
            scs.push_back(instr.lhs().m_block);
            scs.push_back(instr.rhs().m_block);
            break;
        }
        default: {
            throw CodegenError("[BasicBlock] Block '" + this->m_label + "' does not end in a terminator");
        }
    }

    return scs;
}

} // namespace blast::core::ir
