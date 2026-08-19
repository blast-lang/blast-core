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
        case Operand::Kind::LITERAL:  return std::to_string(op.m_imm);
    }
    return "?";
}

} // namespace


Operand ASTtoTAC::visitIntLiteral(const parser::IntLiteral& node) {
    return INT_LIT(node.value());
}

Operand ASTtoTAC::visitBinaryExpr(const parser::BinaryExpr& node) {
    const Operand lhs = this->visit(node.lhs());
    const Operand rhs = this->visit(node.rhs());
    // Generate a new valueId (register) for the current function
    const ValueId new_vid = static_cast<ValueId>(this->m_fn.m_valueTypes.size() - 1);
    // And create the instruction for it
    Instruction instr{REGISTER(new_vid), lhs, rhs, opcodeFor(node.op())};
    // And store it as part of current block's instructions
    
    // And return this new register
    return REGISTER(new_vid);
}

Operand ASTtoTAC::visitIdentifier(const parser::Identifier& node) {
    const auto it = this->m_vars.find(node.name());
    if (it == this->m_vars.end()) {
        throw CodegenError("unknown name '" + node.name() + "'");
    }
    return it->second;
}

/*
Operand ASTtoTAC::visitVarDecl(const parser::VarDecl& node) {
    const Operand value = node.hasInit() ? this->visit(node.init()) : NONE();
    this->m_vars[node.name()] = value;
    return value;
}

Operand ASTtoTAC::visitAssign(const parser::Assign& node) {
    if (node.target()->kind() != parser::ASTNode::Kind::Identifier) {
        throw CodegenError("assignment target must be a name");
    }
    const auto& name = static_cast<const parser::Identifier*>(node.target())->name();
    const Operand value = this->visit(node.value());
    this->m_vars[name] = value;
    return value;
}*/

Operand ASTtoTAC::visitExprStmt(const parser::ExprStmt& node) {
    return this->visit(node.expr());
}

Operand ASTtoTAC::visitTranslationUnit(const parser::TranslationUnit& node) {
    Operand last = NONE();
    for (const auto& stmt : node.stmts()) {
        last = this->visit(stmt.get());
    }
    return last;
}

Function ASTtoTAC::run(const parser::TranslationUnit& unit) {
    this->m_fn.m_name = "blast_main";
    this->m_fn.m_type = nullptr;
    this->m_fn.m_blocks.emplace_back();

    const Operand result = this->visit(&unit);
    this->m_fn.m_blocks[0].m_instrs.push_back(
        Instruction{NONE(), result, NONE(), Opcode::RET});

    return std::move(this->m_fn);
}

std::string dump(const Function& fn) {
    std::string out = "function @" + fn.m_name + "\nblock0:\n";
    for (const Instruction& instr : fn.m_blocks[0].m_instrs) {
        out += "    ";
        if (instr.m_result.m_kind != Operand::Kind::NONE) {
            out += operandText(instr.m_result) + " = ";
        }
        out += opcodeName(instr.m_op);
        if (instr.m_lhs.m_kind != Operand::Kind::NONE) {
            out += " " + operandText(instr.m_lhs);
        }
        if (instr.m_rhs.m_kind != Operand::Kind::NONE) {
            out += ", " + operandText(instr.m_rhs);
        }
        out += "\n";
    }
    return out;
}

} // namespace blast::core::ir::tac
