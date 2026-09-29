#include <core/ir/IR.hpp>
#include <core/Exception.hpp>
#include <iterator>
#include <set>

namespace blast::core::ir {

namespace {

Opcode opcodeFor(const std::string& op) {
    if (op == "+") return Opcode::ADD;
    if (op == "*") return Opcode::MUL;
    if (op == ">") return Opcode::GT;
    if (op == ">=") return Opcode::GE;
    if (op == "<") return Opcode::LT;
    if (op == "<=") return Opcode::LE;
    if (op == "==") return Opcode::EQ;
    if (op == "!=") return Opcode::NE;
    throw CodegenError("unsupported operator '" + op + "'");
}

} // namespace


void SSAIR::setLKO(context::Symbol* s, BlockId bid, const Operand& o) {
    if (s) {
        this->m_lko.insert_or_assign(
            std::make_pair(s->id(), bid), o
        );
    }
}


void SSAIR::fillPhi(context::Symbol* s, BlockId bid, std::size_t idx) {
    const std::vector<BlockId> preds = this->currentFct().getBlock(bid).preds();
    for (BlockId pred: preds) {
        const Operand incoming = this->getLKO(s, pred);
        this->currentFct().getBlock(bid).phis()[idx].addIncoming(pred, incoming);
    }
}

void SSAIR::sealBlock(BasicBlock& b) {
    const std::vector<std::pair<context::Symbol*, std::size_t>> incomplete = this->m_incomplete[b.id()];
    for (const auto& [s, idx]: incomplete) {
        this->fillPhi(s, b.id(), idx);
    }
    this->m_incomplete.erase(b.id());
    b.seal();
}

Operand SSAIR::getLKORecurvise(context::Symbol* s, BlockId bid) {
    Operand v = NONE();
    const BasicBlock& b = this->currentFct().getBlock(bid);
    const std::vector<BlockId> preds = b.preds();
    // Are we looking for an LKO in a unsealed block?
    // I.e block where we know the predecessor list if not yet fixed.
    // This is needed when dependency is circular: %2 = PHI [..., %1] needs %1, and %1 = ADD %2, 1 needs %2
    // It happens with loops
    if (!b.sealed()) {
        // Add an empty phi to be filled later when we 'seal' the block
        // This is just to reserve the slot for 'v'
        v = this->currentFct().addPhi(bid, fromContextType(s->type()));
        this->m_incomplete[bid].push_back({s, b.phis().size() - 1});
        this->setLKO(s, bid, v);
        return v;
    }
    // No preds
    else if (preds.empty()) {
        return NONE();
    }
    // One predecessor, upstream block should have LKO
    else if (preds.size() == 1) {
        v = this->getLKO(s, preds[0]);
        this->setLKO(s, bid, v);
        return v;
    }
    // Several predecessors, and block sealed:
    // Add fixed (seal) phis
    v = this->currentFct().addPhi(bid, fromContextType(s->type()));
    this->setLKO(s, bid, v);
    this->fillPhi(s, bid, b.phis().size() - 1);

    // This algorithm may create trivial phis like:
    //   i64 %3 = PHI [entry: i64 0], [while1.body: i64 %3]
    //   i64 %1 = LT i64 %0, i64 100
    // while1.body:
    //   i64 %2 = ADD i64 %0, i64 1
    //
    // We have a phi for %3 that is not set in body
    // But can remove it during optimization pass
    return v;
}


// The binding for 's' as seen from 'bid': the one recorded there, else the
// one its predecessors carry, else a phi picking between them.
Operand SSAIR::getLKO(context::Symbol* s, BlockId bid) {
    if (s == nullptr) {
        return NONE();
    }

    // Can we find a reference of this symbol in block 'bid' ?
    // If so, return it
    // Otherwise search for it recursivly in the predecessor chain
    auto it = this->m_lko.find(std::make_pair(s->id(), bid));
    if (it != this->m_lko.end()) {
        return it->second;
    } else {
        return getLKORecurvise(s, bid);
    }
    
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
    this->setLKO(v, this->m_current_block, reg);
    return reg;
}

Operand SSAIR::visitAssign(const parser::Assign& node) {
    const Operand rhs = this->visit(node.value());
    context::Symbol* s_lhs = this->m_ctx->getNodeSymbol(node.target());
    this->setLKO(s_lhs, this->m_current_block, rhs);
    return rhs;
}

Operand SSAIR::visitExprStmt(const parser::ExprStmt& node) {
    return this->visit(node.expr());
}


Operand SSAIR::visitIfStmt(const parser::IfStmt& node) {
    Operand cond = this->visit(node.cond());
    const BlockId cond_id = this->m_current_block;

    const std::string suffix = std::to_string(this->currentFct().blocks().size());
    // Create the 'then' block as predecessor of the 'cond' block
    const BlockId then_id = this->currentFct().addBlock("if" + suffix + ".then");
    // Create the 'join' block, that is reach after 'then' or if 'cond' is false
    const BlockId join_id = this->currentFct().addBlock("if" + suffix + ".join");
    BlockId else_id = 0;
    
    // Connect 'cond' block to 'then' block
    this->currentFct().getBlock(then_id).preds().push_back(cond_id);
    this->sealBlock(this->currentFct().getBlock(then_id));

    // Connect 'join' or 'else' depending if there's an else at all
    if (node.hasElse()) {
        else_id = this->currentFct().addBlock("if" + suffix + ".else");
        this->currentFct().getBlock(else_id).preds().push_back(cond_id);
        this->sealBlock(this->currentFct().getBlock(else_id));
        // if a goto 'then' else goto 'else'
        this->currentFct().addCBR(cond_id, cond, then_id, else_id);
    } else {
        this->currentFct().getBlock(join_id).preds().push_back(cond_id);
        // if a goto 'then' else goto 'join'
        this->currentFct().addCBR(cond_id, cond, then_id, join_id);
    }

    // Move to the new 'then' block to visit it
    this->m_current_block = then_id;
    this->visit(node.thenBranch());

    // A nested 'if' leaves us in its own join, so branch from where we landed.
    const BlockId then_end = this->m_current_block;
    this->currentFct().getBlock(join_id).preds().push_back(then_end);
    // Unconditional jump from 'then' to 'join'
    this->currentFct().addBR(then_end, join_id);

    // Same with 'else' block
    if (node.hasElse()) {
        this->m_current_block = else_id;
        this->visit(node.elseBranch());

        const BlockId else_end = this->m_current_block;
        this->currentFct().getBlock(join_id).preds().push_back(else_end);
        this->currentFct().addBR(else_end, join_id);
    }
    this->sealBlock(this->currentFct().getBlock(join_id));

    // Continue forward after 'join' block
    this->m_current_block = join_id;
    return NONE();
}

Operand SSAIR::visitWhileStmt(const parser::WhileStmt& node) {
    const std::string suffix = std::to_string(this->currentFct().blocks().size());

    // Condition block, block following the current block
    const BlockId cond_id = this->currentFct().addBlock("while" + suffix + ".cond");
    // Current block jumps directly to the condition
    this->currentFct().addBR(this->m_current_block, cond_id);
    this->currentFct().getBlock(cond_id).preds().push_back(this->m_current_block);

    // Visit condition
    this->m_current_block = cond_id;
    Operand cond = this->visit(node.cond());
    
    // Create the 'body' block as predecessor of the 'cond' block
    const BlockId body_id = this->currentFct().addBlock("while" + suffix + ".body");
    this->currentFct().getBlock(body_id).preds().push_back(cond_id);
    this->sealBlock(this->currentFct().getBlock(body_id));
    // Create the 'join' block, that is reached atfer the 'cond' condition
    const BlockId join_id = this->currentFct().addBlock("while" + suffix + ".join");
    this->currentFct().getBlock(join_id).preds().push_back(cond_id);
    this->sealBlock(this->currentFct().getBlock(join_id));

    // Condition evaluation
    this->currentFct().addCBR(cond_id, cond, body_id, join_id);

    // Visit body
    this->m_current_block = body_id;
    this->visit(node.body());
    // Usefull in case of nested statements
    const BlockId body_end = this->m_current_block;
    // Jump from body to condition
    this->currentFct().addBR(body_end, cond_id);
    this->currentFct().getBlock(cond_id).preds().push_back(body_end);
    this->sealBlock(this->currentFct().getBlock(cond_id));

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
    this->sealBlock(this->currentFct().getBlock(0));
    const Operand last = this->visit(&unit);
    // Return 0
    this->addInstruction(LITERAL(std::int64_t{0}), NONE(), Opcode::RET);
    for (Function& fct: this->m_main.fcts()) {
        resolveCriticalEdges(fct);
    }
    return this->m_main;
}

void SSAIR::resolveCriticalEdges(Function& fct) {
    // We assume at this point that the control flow graph if 'fct' is complete and all predecessors are set
    // This function will:
    // Compute successors for o(1) access
    // split critical edges: https://nickdesaulniers.github.io/blog/2023/01/27/critical-edge-splitting/
    // Resolve phi with assignement, that will break SSA (!!!)
    std::unordered_map<BlockId, std::set<BlockId>> predecessors;
    std::unordered_map<BlockId, std::set<BlockId>> successors;

    // Find the successor blocks of a given block by looking at its termination instruction
    auto computeSuccessors = [](const BasicBlock& block) {
        std::set<BlockId> scs;
        if (block.instrs().empty()) {
            return scs;
        }

        const Instruction& instr = block.instrs().back();

        switch (instr.op()) {
            case Opcode::RET: {
                break;
            }
            case Opcode::BR: {
                scs.insert(instr.lhs().m_block);
                break;
            }
            case Opcode::CBR: {
                scs.insert(instr.lhs().m_block);
                scs.insert(instr.rhs().m_block);
                break;
            }
            default: {
                throw CodegenError("[BasicBlock] Block '" + block.label() + "' does not end in a terminator");
            }
        }

        return scs;
    };

    for (BasicBlock& block: fct.blocks()) {
        predecessors[block.id()] = std::set<BlockId>(block.preds().begin(), block.preds().end());
        successors[block.id()] = computeSuccessors(block);
    }

    // Find critical edge: block who's predecessor has multiple successors
    // AND this block as mulltiple predecessors
    std::set<std::pair<BlockId, BlockId>> critical_edges;
    for (BasicBlock& block: fct.blocks()) {
        if (predecessors[block.id()].size() > 1) {
            for (BlockId pred: predecessors[block.id()]) {
                if (successors[pred].size() > 1) {
                    critical_edges.insert({pred, block.id()});
                }
            }
        }
    }

    for (const auto& [i,j]: critical_edges) {
        // remove edge i -> j and create a empty block node so that we have i -> b{} -> j
        successors[i].erase(j);
        predecessors[j].erase(i);
        BlockId newblock = fct.addBlock(fct.getBlock(i).label() + "." + fct.getBlock(j).label());
        // Create the two new non-critical edges
        successors[i].insert(newblock);
        predecessors[j].insert(newblock);
        successors[newblock].insert(j);
        predecessors[newblock].insert(i);

        // Now the termitators (jump instructions) of the blocks i and j
        // Needs to be changes to take newblock into account
        Instruction& term = fct.getBlock(i).instrs().back();
        Operand lhs = term.lhs();
        Operand rhs = term.rhs();
        if (lhs.m_block == j) {
            lhs = BLOCK(newblock);
        }
        if (rhs.m_block == j) {
            rhs = BLOCK(newblock);
        }
        const std::string comment = term.comment();
        term = Instruction(term.result(), lhs, rhs, term.op());
        term.setComment(comment);
        fct.addBR(newblock, j);

        // Same thing for the phis
        for (Phi& phi: fct.getBlock(j).phis()) {
            for (auto& [from, value]: phi.incomings()) {
                if (from == i) {
                    from = newblock;
                }
            }
        }
    }

    // Now, correcttly fill each block's successors and predecessors
    for (BasicBlock& block: fct.blocks()) {
        const std::set<BlockId>& preds = predecessors[block.id()];
        const std::set<BlockId>& succs = successors[block.id()];
        block.preds().assign(preds.begin(), preds.end());
        block.successors().assign(succs.begin(), succs.end());
    }

}

} // namespace blast::core::ir
