#include <core/ir/TAC.hpp>
#include <core/Exception.hpp>

namespace blast::core::ir::tac {

namespace {

Opcode opcodeFor(const std::string& op) {
    if (op == "+") return Opcode::ADD;
    if (op == "*") return Opcode::MUL;
    throw CodegenError("unsupported operator '" + op + "'");
}

const char* opcodeName(Opcode op) {
    switch (op) {
        case Opcode::ADD: return "ADD";
        case Opcode::SUB: return "SUB";
        case Opcode::MUL: return "MUL";
        case Opcode::DIV: return "DIV";
        case Opcode::NEG: return "NEG";
        case Opcode::LT:  return "LT";
        case Opcode::LE:  return "LE";
        case Opcode::GT:  return "GT";
        case Opcode::GE:  return "GE";
        case Opcode::EQ:  return "EQ";
        case Opcode::NE:  return "NE";
        case Opcode::COPY: return "COPY";
        case Opcode::CALL: return "CALL";
        case Opcode::BR:  return "BR";
        case Opcode::CBR: return "CBR";
        case Opcode::RET: return "RET";
    }
    return "?";
}

std::string operandText(const Operand& op) {
    switch (op.m_kind) {
        case Operand::Kind::NONE:     return "";
        case Operand::Kind::BLOCK:    return "block_" + std::to_string(op.m_block);
        case Operand::Kind::REGISTER: return "%" + std::to_string(op.m_value);
        case Operand::Kind::LITERAL:  return std::to_string(op.m_i64);
    }
    return "?";
}

// "%0 = ADD %1, 2" -- the result is dropped for instructions that produce no
// value (the terminators), and an operand left NONE is simply not printed.
std::string instructionText(const Instruction& instr) {
    std::string text;
    if (instr.result().m_kind != Operand::Kind::NONE) {
        text += operandText(instr.result()) + " = ";
    }
    text += opcodeName(instr.op());
    if (instr.lhs().m_kind != Operand::Kind::NONE) {
        text += " " + operandText(instr.lhs());
    }
    if (instr.rhs().m_kind != Operand::Kind::NONE) {
        text += ", " + operandText(instr.rhs());
    }
    if (!instr.comment().empty()) {
        text += "  // " + instr.comment();
    }
    return text;
}

} // namespace


Operand ASTtoTAC::visitIntLiteral(const parser::IntLiteral& node) {
    return INT_LIT(node.value());
}

// Operand's LITERAL payload is an int64, so there is nowhere to put these yet.
// Fail loudly rather than lower them to something they are not: a silent
// visitEmpty() here is indistinguishable from a correctly empty block.
Operand ASTtoTAC::visitFloatLiteral(const parser::FloatLiteral&) {
    throw CodegenError("float literals are not lowered yet");
}

Operand ASTtoTAC::visitBoolLiteral(const parser::BoolLiteral&) {
    throw CodegenError("bool literals are not lowered yet");
}

Operand ASTtoTAC::visitStringLiteral(const parser::StringLiteral&) {
    throw CodegenError("string literals are not lowered yet");
}

Operand ASTtoTAC::visitIdentifier(const parser::Identifier& node) {
    const auto it = this->m_vars.find(node.name());
    if (it == this->m_vars.end()) {
        throw CodegenError("unbound identifier '" + node.name() + "'");
    }
    return it->second;
}

Operand ASTtoTAC::visitBinaryExpr(const parser::BinaryExpr& node) {
    const Operand lhs = this->visit(node.lhs());
    const Operand rhs = this->visit(node.rhs());
    return this->currentBlock().newInstruction(lhs, rhs, opcodeFor(node.op()));
}

// The declared type is ignored here: checking it against the initialiser is
// semantic analysis' job, and this pass runs after it.
Operand ASTtoTAC::visitVarDecl(const parser::VarDecl& node) {
    if (!node.hasInit()) {
        // Declared but not initialised. Leaving the name unbound makes a read
        // before the first assignment an error in visitIdentifier.
        return NONE();
    }
    const Operand value = this->visit(node.init());
    this->m_vars[node.name()] = value;
    return value;
}

Operand ASTtoTAC::visitAssign(const parser::Assign& node) {
    const parser::Expr* target = node.target();
    if (target->kind() != parser::ASTNode::Kind::Identifier) {
        throw CodegenError("assignment target is not a name");
    }
    const Operand value = this->visit(node.value());
    // In SSA an assignment does not copy: it rebinds the name to the value the
    // right-hand side already produced, so no instruction is emitted here.
    this->m_vars[static_cast<const parser::Identifier*>(target)->name()] = value;
    return value;
}

Operand ASTtoTAC::visitExprStmt(const parser::ExprStmt& node) {
    return this->visit(node.expr());
}

Operand ASTtoTAC::visitTranslationUnit(const parser::TranslationUnit& node) {
    for (const auto& stmt : node.stmts()) {
        this->visit(stmt.get());
    }
    // A translation unit is not an expression: it has no value.
    return NONE();
}

Function ASTtoTAC::run(const parser::TranslationUnit& unit) {
    this->visit(&unit);
    return this->currentFct();
}

std::string dump(const Function& fn) {
    std::string text = "fn @" + fn.name() + " {\n";
    for (const BasicBlock& block : fn.blocks()) {
        text += "block_" + std::to_string(block.id()) + ":\n";
        for (const Instruction& instr : block.instrs()) {
            text += "  " + instructionText(instr) + "\n";
        }
    }
    text += "}\n";
    return text;
}

} // namespace blast::core::ir::tac
