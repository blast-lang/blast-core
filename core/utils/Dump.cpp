#include <core/utils/Dump.hpp>
#include <core/codegen/IRVisitor.hpp>
#include <core/parser/AstVisitor.hpp>
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
    // Restore alongside IfStmt in Ast.hpp:
    // std::string visitIfStmt(const IfStmt& n) {
    //     std::string out = "(if " + visit(n.cond()) + " " + visit(n.thenBranch());
    //     if (n.hasElse()) {
    //         out += " else " + visit(n.elseBranch());
    //     }
    //     return out + ")";
    // }
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
    // std::string visitIfStmt(const IfStmt&)              { return "IfStmt"; }
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
    // Restore alongside IfStmt in Ast.hpp:
    // std::vector<Child> visitIfStmt(const IfStmt& n) {
    //     std::vector<Child> children{{"cond: ", n.cond()}, {"then: ", n.thenBranch()}};
    //     if (n.hasElse()) {
    //         children.push_back({"else: ", n.elseBranch()});
    //     }
    //     return children;
    // }
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
        case ir::Opcode::BR:   return "BR";
        case ir::Opcode::CBR:  return "CBR";
        case ir::Opcode::RET:  return "RET";
    }
    return "?";
}

std::string operandText(const ir::Operand& op) {
    switch (op.m_kind) {
        case ir::Operand::Kind::NONE:     return "";
        case ir::Operand::Kind::BLOCK:    return "block_" + std::to_string(op.m_block);
        case ir::Operand::Kind::REGISTER: return "%" + std::to_string(op.m_value);
        case ir::Operand::Kind::LITERAL:  return std::to_string(op.m_i64);
    }
    return "?";
}

// "%0 = ADD %1, 2" -- the result is dropped for instructions that produce no
// value (the terminators), and an operand left NONE is simply not printed.
std::string instructionText(const ir::Instruction& instr) {
    std::string text;
    if (instr.result().m_kind != ir::Operand::Kind::NONE) {
        text += operandText(instr.result()) + " = ";
    }
    text += opcodeName(instr.op());
    bool has_operand = false;
    if (instr.lhs().m_kind != ir::Operand::Kind::NONE) {
        text += " " + operandText(instr.lhs());
        has_operand = true;
    }
    if (instr.rhs().m_kind != ir::Operand::Kind::NONE) {
        text += (has_operand ? ", " : " ") + operandText(instr.rhs());
    }
    if (!instr.comment().empty()) {
        text += "  // " + instr.comment();
    }
    return text;
}

// Each level frames its children and delegates the descent to the base.
class IRDumpVisitor: public codegen::IRVisitor<IRDumpVisitor> {
    using Base = codegen::IRVisitor<IRDumpVisitor>;

public:
    void visitFunction(const ir::Function& fn) {
        this->m_text += "fn @" + fn.name() + " {\n";
        this->Base::visitFunction(fn);
        this->m_text += "}\n";
    }

    void visitBlock(const ir::BasicBlock& block) {
        this->m_text += "block_" + std::to_string(block.id()) + ":\n";
        this->Base::visitBlock(block);
    }

    void visitInstruction(const ir::Instruction& instr) {
        this->m_text += "  " + instructionText(instr) + "\n";
    }

    std::string& text() { return this->m_text; }

private:
    std::string m_text;
};

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
    IRDumpVisitor visitor;
    visitor.visitFunction(fn);
    return std::move(visitor.text());
}

} // namespace blast::core::utils
