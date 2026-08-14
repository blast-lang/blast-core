#include "Ast.hpp"
#include "AstVisitor.hpp"
#include <utility>

namespace blast::core::parser {

namespace {

// --- s-expression rendering ----------------------------------------------
class DumpVisitor : public AstVisitor<DumpVisitor, std::string> {
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
        std::string out = "(if " + visit(n.cond()) + " " + visit(n.then_branch());
        if (n.has_else()) {
            out += " else " + visit(n.else_branch());
        }
        return out + ")";
    }
    std::string visitContinueStmt(const ContinueStmt&) {
        return "(continue)";
    }
    std::string visitVarDecl(const VarDecl& n) {
        std::string out = "(var-decl " + n.name();
        if (n.has_type()) {
            out += " :: " + visit(n.type());
        }
        if (n.has_init()) {
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
    // optional child -- VarDecl::init(), IfStmt::else_branch().
    std::string visit_empty() { return "<null>"; }

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

    std::string visit_empty() { return "<null>"; }
};

// An edge to a child, with an optional role label ("lhs: ", "type: ", ...).
using Child = std::pair<std::string, const ASTNode*>;

// The labelled edges out of a node, in the order they should be drawn. This is
// the tree-drawing counterpart of ASTVisitor::visit_children(), which walks the
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
        std::vector<Child> children{{"cond: ", n.cond()}, {"then: ", n.then_branch()}};
        if (n.has_else()) {
            children.push_back({"else: ", n.else_branch()});
        }
        return children;
    }
    std::vector<Child> visitVarDecl(const VarDecl& n) {
        std::vector<Child> children;
        if (n.has_type()) {
            children.push_back({"type: ", n.type()});
        }
        if (n.has_init()) {
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

std::string node_label(const ASTNode* node) {
    LabelVisitor labeller;
    return labeller.visit(node);
}

std::vector<Child> node_children(const ASTNode* node) {
    ChildrenVisitor collector;
    return collector.visit(node);
}

// Recursively append each child on its own line. `prefix` carries the vertical
// bars ('|') for the ancestors that still have siblings below them; the last
// child of a parent uses '\__' and drops the bar for its own descendants.
void render(const ASTNode* node, const std::string& prefix, std::string& out) {
    const std::vector<Child> children = node_children(node);
    for (std::size_t i = 0; i < children.size(); ++i) {
        const bool last = (i + 1 == children.size());
        const auto& [label, child] = children[i];
        out += prefix + (last ? "\\__ " : "|__ ") + label + node_label(child) + "\n";
        render(child, prefix + (last ? "    " : "|   "), out);
    }
}

} // namespace

std::string dump(const ASTNode* node) {
    DumpVisitor dumper;
    return dumper.visit(node);
}

std::string dump_tree(const ASTNode* node) {
    std::string out = node_label(node) + "\n";
    render(node, "", out);
    return out;
}

} // namespace blast::core::parser
