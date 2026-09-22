#include <core/utils/Dump.hpp>
#include <core/ir/IR.hpp>
#include <core/parser/AstVisitor.hpp>
#include <cctype>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace blast::core::utils {

using namespace blast::core::parser;

namespace {

// --- s-expression rendering ----------------------------------------------
class DumpVisitor: public AstVisitor<DumpVisitor, std::string> {
public:
    std::string visitIntLiteral(const IntLiteral& n) {
        return "(int " + std::to_string(n.value()) + ")";
    }
    std::string visitFloatLiteral(const FloatLiteral& n) {
        return "(float " + std::to_string(n.value()) + ")";
    }
    std::string visitBoolLiteral(const BoolLiteral& n) {
        return std::string("(bool ") + (n.value() ? "true" : "false") + ")";
    }
    std::string visitStringLiteral(const StringLiteral& n) {
        return "(str \"" + n.value() + "\")";
    }
    std::string visitIdentifier(const Identifier& n) {
        return "(id " + n.name() + ")";
    }
    std::string visitUnaryExpr(const UnaryExpr& n) {
        return "(unary " + n.op() + " " + visit(n.operand()) + ")";
    }
    std::string visitBinaryExpr(const BinaryExpr& n) {
        return "(binary " + n.op() + " " + visit(n.lhs()) + " " + visit(n.rhs()) + ")";
    }
    std::string visitAssign(const Assign& n) {
        return "(assign " + visit(n.target()) + " " + visit(n.value()) + ")";
    }
    std::string visitExprStmt(const ExprStmt& n) {
        return "(expr-stmt " + visit(n.expr()) + ")";
    }
    std::string visitIfStmt(const IfStmt& n) {
        std::string out = "(if " + visit(n.cond()) + " " + visit(n.thenBranch());
        if (n.hasElse()) {
            out += " else " + visit(n.elseBranch());
        }
        return out + ")";
    }
    std::string visitContinueStmt(const ContinueStmt&) {
        return "(continue)";
    }
    std::string visitVarDecl(const VarDecl& n) {
        std::string out = "(var-decl " + n.name();
        if (n.hasType()) {
            out += " :: " + visit(n.type());
        }
        if (n.hasInit()) {
            out += " = " + visit(n.init());
        }
        return out + ")";
    }
    std::string visitBlock(const Block& n) {
        return join("(block", n.stmts());
    }
    std::string visitTranslationUnit(const TranslationUnit& n) {
        return join("(unit", n.stmts());
    }

    // Every kind above is handled, so this is only reached for an absent
    // optional child -- VarDecl::init(), IfStmt::elseBranch().
    std::string visitEmpty() { return "<null>"; }

private:
    std::string join(std::string out, const std::vector<std::unique_ptr<Stmt>>& stmts) {
        for (const auto& stmt : stmts) {
            out += " " + visit(stmt.get());
        }
        return out + ")";
    }
};

// --- ASCII tree rendering ------------------------------------------------
// The node's own one-line label: its kind plus the payload it carries
// directly, with no recursion into children.
class LabelVisitor : public AstVisitor<LabelVisitor, std::string> {
public:
    std::string visitIntLiteral(const IntLiteral& n) {
        return "IntLiteral " + std::to_string(n.value());
    }
    std::string visitFloatLiteral(const FloatLiteral& n) {
        return "FloatLiteral " + std::to_string(n.value());
    }
    std::string visitBoolLiteral(const BoolLiteral& n) {
        return std::string("BoolLiteral ") + (n.value() ? "true" : "false");
    }
    std::string visitStringLiteral(const StringLiteral& n) {
        return "StringLiteral \"" + n.value() + "\"";
    }
    std::string visitIdentifier(const Identifier& n) {
        return "Identifier " + n.name();
    }
    std::string visitUnaryExpr(const UnaryExpr& n) {
        return "UnaryExpr '" + n.op() + "'";
    }
    std::string visitBinaryExpr(const BinaryExpr& n) {
        return "BinaryExpr '" + n.op() + "'";
    }
    std::string visitAssign(const Assign&)                 { return "Assign"; }
    std::string visitExprStmt(const ExprStmt&)             { return "ExprStmt"; }
    std::string visitIfStmt(const IfStmt&)                 { return "IfStmt"; }
    std::string visitContinueStmt(const ContinueStmt&)     { return "ContinueStmt"; }
    std::string visitBlock(const Block&)                   { return "Block"; }
    std::string visitVarDecl(const VarDecl& n)             { return "VarDecl '" + n.name() + "'"; }
    std::string visitTranslationUnit(const TranslationUnit&) { return "TranslationUnit"; }

    std::string visitEmpty() { return "<null>"; }
};

// An edge to a child, with an optional role label ("lhs: ", "type: ", ...).
using Child = std::pair<std::string, const ASTNode*>;

// The labelled edges out of a node, in the order they should be drawn. This is
// the tree-drawing counterpart of ASTVisitor::visitChildren(), which walks the
// same edges but discards the labels.
class ChildrenVisitor : public AstVisitor<ChildrenVisitor, std::vector<Child>> {
public:
    std::vector<Child> visitUnaryExpr(const UnaryExpr& n) {
        return {{"", n.operand()}};
    }
    std::vector<Child> visitBinaryExpr(const BinaryExpr& n) {
        return {{"lhs: ", n.lhs()}, {"rhs: ", n.rhs()}};
    }
    std::vector<Child> visitAssign(const Assign& n) {
        return {{"target: ", n.target()}, {"value: ", n.value()}};
    }
    std::vector<Child> visitExprStmt(const ExprStmt& n) {
        return {{"", n.expr()}};
    }
    std::vector<Child> visitIfStmt(const IfStmt& n) {
        std::vector<Child> children{{"cond: ", n.cond()}, {"then: ", n.thenBranch()}};
        if (n.hasElse()) {
            children.push_back({"else: ", n.elseBranch()});
        }
        return children;
    }
    std::vector<Child> visitVarDecl(const VarDecl& n) {
        std::vector<Child> children;
        if (n.hasType()) {
            children.push_back({"type: ", n.type()});
        }
        if (n.hasInit()) {
            children.push_back({"init: ", n.init()});
        }
        return children;
    }
    std::vector<Child> visitBlock(const Block& n) {
        return sequence(n.stmts());
    }
    std::vector<Child> visitTranslationUnit(const TranslationUnit& n) {
        return sequence(n.stmts());
    }

private:
    std::vector<Child> sequence(const std::vector<std::unique_ptr<Stmt>>& stmts) {
        std::vector<Child> children;
        children.reserve(stmts.size());
        for (const auto& stmt : stmts) {
            children.push_back({"", stmt.get()});
        }
        return children;
    }
};

std::string nodeLabel(const ASTNode* node) {
    LabelVisitor labeller;
    return labeller.visit(node);
}

std::vector<Child> nodeChildren(const ASTNode* node) {
    ChildrenVisitor collector;
    return collector.visit(node);
}

// Recursively append each child on its own line. `prefix` carries the vertical
// bars ('|') for the ancestors that still have siblings below them; the last
// child of a parent uses '\__' and drops the bar for its own descendants.
void render(const ASTNode* node, const std::string& prefix, std::string& out) {
    const std::vector<Child> children = nodeChildren(node);
    for (std::size_t i = 0; i < children.size(); ++i) {
        const bool last = (i + 1 == children.size());
        const auto& [label, child] = children[i];
        out += prefix + (last ? "\\__ " : "|__ ") + label + nodeLabel(child) + "\n";
        render(child, prefix + (last ? "    " : "|   "), out);
    }
}

// --- IR rendering --------------------------------------------------------
const char* opcodeName(ir::Opcode op) {
    switch (op) {
        case ir::Opcode::ADD:  return "ADD";
        case ir::Opcode::SUB:  return "SUB";
        case ir::Opcode::MUL:  return "MUL";
        case ir::Opcode::DIV:  return "DIV";
        case ir::Opcode::NEG:  return "NEG";
        case ir::Opcode::LT:   return "LT";
        case ir::Opcode::LE:   return "LE";
        case ir::Opcode::GT:   return "GT";
        case ir::Opcode::GE:   return "GE";
        case ir::Opcode::EQ:   return "EQ";
        case ir::Opcode::NE:   return "NE";
        case ir::Opcode::COPY: return "COPY";
        case ir::Opcode::CALL: return "CALL";
        case ir::Opcode::ALLOCA: return "ALLOCA";
        case ir::Opcode::LOAD: return "LOAD";
        case ir::Opcode::STORE: return "STORE";
        case ir::Opcode::BR:   return "BR";
        case ir::Opcode::CBR:  return "CBR";
        case ir::Opcode::RET:  return "RET";
    }
    return "?";
}

std::string typeName(ir::Type t) {
    switch (t.m_kind) {
        case ir::Type::Kind::INT:   return "i" + std::to_string(ir::bits(t));
        case ir::Type::Kind::UINT:  return "u" + std::to_string(ir::bits(t));
        case ir::Type::Kind::FLOAT: return "f" + std::to_string(ir::bits(t));
        case ir::Type::Kind::PTR:   return "ptr";
        case ir::Type::Kind::VOID:  return "void";
    }
    return "?";
}

// Blocks are labels, not values: they carry no type worth printing. The owning
// function is what maps the id back to the label the block header prints.
std::string operandText(const ir::Operand& op, const ir::Function& fn) {
    const std::string type = typeName(op.m_type) + " ";
    switch (op.m_kind) {
        case ir::Operand::Kind::NONE:     return "";
        case ir::Operand::Kind::BLOCK:    return fn.getBlock(op.m_block).label();
        case ir::Operand::Kind::REGISTER: return type + "%" + std::to_string(op.m_value);
        case ir::Operand::Kind::LITERAL:
            return type + std::to_string(static_cast<std::int64_t>(op.m_lit.m_i64));
    }
    return "?";
}

// "%0 = ADD %1, 2" -- the result is dropped for instructions that produce no
// value (the terminators), and an operand left NONE is simply not printed.
std::string instructionText(const ir::Instruction& instr, const ir::Function& fn) {
    std::string text;
    // CBR reads its result slot instead of defining it, so it prints as a
    // leading operand rather than as an assignment.
    if (instr.result().m_kind != ir::Operand::Kind::NONE && instr.op() != ir::Opcode::CBR) {
        text += operandText(instr.result(), fn) + " = ";
    }
    text += opcodeName(instr.op());
    if (instr.op() == ir::Opcode::CBR) {
        text += " " + operandText(instr.result(), fn) + ",";
    }
    bool has_operand = false;
    if (instr.lhs().m_kind != ir::Operand::Kind::NONE) {
        text += " " + operandText(instr.lhs(), fn);
        has_operand = true;
    }
    if (instr.rhs().m_kind != ir::Operand::Kind::NONE) {
        text += (has_operand ? ", " : " ") + operandText(instr.rhs(), fn);
    }
    if (!instr.comment().empty()) {
        text += "  // " + instr.comment();
    }
    return text;
}

// "%1 = PHI [entry: i64 1], [if.then: i64 20]" -- one incoming per edge into
// the block, named by the predecessor it arrives from.
std::string phiText(const ir::Phi& phi, const ir::Function& fn) {
    std::string text = operandText(phi.m_result, fn) + " = PHI";
    for (std::size_t i = 0; i < phi.m_incomings.size(); ++i) {
        const auto& incoming = phi.m_incomings[i];
        text += (i > 0 ? ", [" : " [") + fn.getBlock(incoming.first).label()
              + ": " + operandText(incoming.second, fn) + "]";
    }
    return text;
}

std::string regName(const codegen::Register* r) {
    if (r == nullptr) {
        return "none";
    }
    return r->label();
}

std::string machineOpcodeName(codegen::MachineOpcode op) {
    switch (op) {
        case codegen::MachineOpcode::MOV:  return "mov";
        case codegen::MachineOpcode::ADD:  return "add";
        case codegen::MachineOpcode::IMUL: return "imul";
        case codegen::MachineOpcode::XOR:  return "xor";
        case codegen::MachineOpcode::CALL: return "call";
        case codegen::MachineOpcode::PUSH: return "push";
        case codegen::MachineOpcode::POP:  return "pop";
        case codegen::MachineOpcode::RET:  return "ret";
        case codegen::MachineOpcode::LEA:  return "lea";
    }
    return "?";
}

// Virtual registers keep the IR's '%' spelling so a lowered instruction can be
// read against the IR it came from.
std::string machineOperandText(const codegen::MachineOperand& op) {
    switch (op.m_kind) {
        case codegen::MachineOperand::Kind::NONE: return "";
        case codegen::MachineOperand::Kind::VREG: return "%" + std::to_string(op.m_vreg);
        case codegen::MachineOperand::Kind::PREG: return regName(op.m_preg);
        case codegen::MachineOperand::Kind::SYM:  return op.m_sym;
        case codegen::MachineOperand::Kind::RIP:  return op.m_sym + "(rip)";
        case codegen::MachineOperand::Kind::LIT:
            return std::to_string(static_cast<std::int64_t>(op.m_lit.m_i64));
    }
    return "?";
}

std::string machineInstructionText(const codegen::MachineInstruction& instr) {
    std::string text = machineOpcodeName(instr.m_op);
    bool has_operand = false;
    if (instr.m_dst.m_kind != codegen::MachineOperand::Kind::NONE) {
        text += " " + machineOperandText(instr.m_dst);
        has_operand = true;
    }
    if (instr.m_src.m_kind != codegen::MachineOperand::Kind::NONE) {
        text += (has_operand ? ", " : " ") + machineOperandText(instr.m_src);
    }
    return text;
}

// --- AT&T rendering ------------------------------------------------------
char widthSuffix(ir::Type type) {
    switch (type.m_width) {
        case ir::Type::Width::W8:  return 'b';
        case ir::Type::Width::W16: return 'w';
        case ir::Type::Width::W32: return 'l';
        case ir::Type::Width::W64: return 'q';
        default:
            throw CodegenError("[emit] No AT&T suffix for width " + std::to_string(ir::bits(type)));
    }
}

std::string mnemonic(codegen::MachineOpcode op) {
    const std::string name = machineOpcodeName(op);
    if (name == "?") {
        throw CodegenError("[emit] Unknown opcode");
    }
    return name;
}

std::string attOperandText(const codegen::MachineOperand& op) {
    switch (op.m_kind) {
        case codegen::MachineOperand::Kind::PREG: {
            if (op.m_preg == nullptr) {
                throw CodegenError("[emit] Physical register operand without a register");
            }
            std::string text = op.m_preg->label();
            for (char& c : text) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
            return "%" + text;
        }
        case codegen::MachineOperand::Kind::SYM:
            return op.m_sym;
        case codegen::MachineOperand::Kind::RIP:
            return op.m_sym + "(%rip)";
        case codegen::MachineOperand::Kind::LIT:
            return "$" + std::to_string(static_cast<std::int64_t>(op.m_lit.m_i64));
        case codegen::MachineOperand::Kind::VREG:
            throw CodegenError("[emit] Virtual register %" + std::to_string(op.m_vreg) + " left at emit time");
        default:
            throw CodegenError("[emit] Unsupported operand kind at emit time");
    }
}

// AT&T puts the source first, and an immediate carries no width of its own, so
// the mnemonic takes its suffix from the destination. A label has no width, so
// a symbolic destination leaves the mnemonic bare.
std::string attInstructionText(const codegen::MachineInstruction& instr) {
    const std::string op = mnemonic(instr.m_op);
    if (instr.m_dst.m_kind == codegen::MachineOperand::Kind::NONE) {
        if (instr.m_src.m_kind == codegen::MachineOperand::Kind::NONE) {
            return op;
        }
        // An instruction that only reads, like push, takes its suffix from the
        // operand it reads.
        std::string read = op;
        if (instr.m_src.m_kind == codegen::MachineOperand::Kind::PREG) {
            read += widthSuffix(instr.m_src.m_type);
        }
        return read + " " + attOperandText(instr.m_src);
    }
    std::string head = op;
    if (instr.m_dst.m_kind == codegen::MachineOperand::Kind::PREG) {
        head += widthSuffix(instr.m_dst.m_type);
    }
    head += " ";
    if (instr.m_src.m_kind == codegen::MachineOperand::Kind::NONE) {
        return head + attOperandText(instr.m_dst);
    }
    return head + attOperandText(instr.m_src) + ", " + attOperandText(instr.m_dst);
}

} // namespace


std::string dump(const lexer::Tokenizer& tokenizer) {
    std::string out;
    for (const lexer::Tokenizer::Token& token : tokenizer.tokens()) {
        out += "[" + std::string(lexer::Tokenizer::kindName(token.m_kind)) + "]   \t"
             + token.m_value + "\n";
    }
    return out;
}

std::string dump(const ASTNode* node) {
    DumpVisitor dumper;
    return dumper.visit(node);
}

std::string dumpTree(const ASTNode* node) {
    std::string out = nodeLabel(node) + "\n";
    render(node, "", out);
    return out;
}

std::string dump(const ir::Function& fn) {
    std::string out = "fn @" + fn.name() + "(";
    for (std::size_t i = 0; i < fn.args().size(); ++i) {
        if (i > 0) {
            out += ", ";
        }
        out += operandText(fn.args()[i], fn);
    }
    out += ") -> " + typeName(fn.ret()) + " {\n";
    for (const ir::BasicBlock& block : fn.blocks()) {
        out += block.label() + ":\n";
        for (const ir::Phi& phi : block.phis()) {
            out += "  " + phiText(phi, fn) + "\n";
        }
        for (const ir::Instruction& instr : block.instrs()) {
            out += "  " + instructionText(instr, fn) + "\n";
        }
    }
    out += "}\n";
    return out;
}

std::string dump(const ir::Module& m) {
    std::string out;
    for (const ir::Function& fn : m.fcts()) {
        out += dump(fn);
    }
    return out;
}

std::string dump(const codegen::MachineFunction& mfn) {
    std::string out;
    for (const codegen::MachineBlock& block : mfn.blocks()) {
        if (!out.empty()) {
            out += "\n";
        }
        out += block.label() + ":\n";
        for (const codegen::MachineInstruction& instr : block.instrs()) {
            out += "  " + machineInstructionText(instr) + "\n";
        }
    }
    return out;
}

std::string dump(const codegen::X86& x86) {
    std::string out;
    for (const codegen::MachineFunction& mfn : x86.fcts()) {
        if (!out.empty()) {
            out += "\n\n";
        }
        out += dump(mfn);
    }
    return out;
}

std::string emit(const codegen::X86& x86) {
    std::string out =
        "    .section .note.GNU-stack,\"\",@progbits\n"
        "\n"
        "    .text\n"
        "    .globl main\n"
        "main:\n";

    for (const codegen::MachineFunction& mfn : x86.fcts()) {
        for (const codegen::MachineBlock& block : mfn.blocks()) {
            out += block.label() + ":\n";
            for (const codegen::MachineInstruction& instr : block.instrs()) {
                out += "    " + attInstructionText(instr) + "\n";
            }
        }
    }

    return out;
}

} // namespace blast::core::utils
