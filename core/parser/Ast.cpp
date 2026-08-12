#include "Ast.hpp"
#include <utility>

namespace blast::core::parser {

std::string dump(const ASTNode* node) {
    if (node == nullptr) {
        return "<null>";
    }

    if (const auto* n = dyn_cast<IntLiteral>(node)) {
        return "(int " + std::to_string(n->value()) + ")";
    }
    if (const auto* n = dyn_cast<FloatLiteral>(node)) {
        return "(float " + std::to_string(n->value()) + ")";
    }
    if (const auto* n = dyn_cast<BoolLiteral>(node)) {
        return std::string("(bool ") + (n->value() ? "true" : "false") + ")";
    }
    if (const auto* n = dyn_cast<StringLiteral>(node)) {
        return "(str \"" + n->value() + "\")";
    }
    if (const auto* n = dyn_cast<Identifier>(node)) {
        return "(id " + n->name() + ")";
    }
    if (const auto* n = dyn_cast<UnaryExpr>(node)) {
        return "(unary " + n->op() + " " + dump(n->operand()) + ")";
    }
    if (const auto* n = dyn_cast<BinaryExpr>(node)) {
        return "(binary " + n->op() + " " + dump(n->lhs()) + " " + dump(n->rhs()) + ")";
    }
    if (const auto* n = dyn_cast<Assign>(node)) {
        return "(assign " + dump(n->target()) + " " + dump(n->value()) + ")";
    }
    if (const auto* n = dyn_cast<ExprStmt>(node)) {
        return "(expr-stmt " + dump(n->expr()) + ")";
    }
    if (const auto* n = dyn_cast<IfStmt>(node)) {
        std::string out = "(if " + dump(n->cond()) + " " + dump(n->then_branch());
        if (n->else_branch() != nullptr) {
            out += " else " + dump(n->else_branch());
        }
        return out + ")";
    }
    if (isa<ContinueStmt>(node)) {
        return "(continue)";
    }
    if (const auto* n = dyn_cast<VarDecl>(node)) {
        std::string out = "(var-decl " + n->name();
        if (n->type() != nullptr) {
            out += " :: " + dump(n->type());
        }
        if (n->init() != nullptr) {
            out += " = " + dump(n->init());
        }
        return out + ")";
    }
    if (const auto* n = dyn_cast<Block>(node)) {
        std::string out = "(block";
        for (const auto& stmt : n->stmts()) {
            out += " " + dump(stmt.get());
        }
        return out + ")";
    }
    if (const auto* n = dyn_cast<TranslationUnit>(node)) {
        std::string out = "(unit";
        for (const auto& stmt : n->stmts()) {
            out += " " + dump(stmt.get());
        }
        return out + ")";
    }

    return "<unknown>";
}

// --- ASCII tree rendering ------------------------------------------------
namespace {

// The node's own one-line label (its kind + immediate payload).
std::string node_label(const ASTNode* node) {
    if (node == nullptr) {
        return "<null>";
    }
    if (const auto* n = dyn_cast<IntLiteral>(node)) {
        return "IntLiteral " + std::to_string(n->value());
    }
    if (const auto* n = dyn_cast<FloatLiteral>(node)) {
        return "FloatLiteral " + std::to_string(n->value());
    }
    if (const auto* n = dyn_cast<BoolLiteral>(node)) {
        return std::string("BoolLiteral ") + (n->value() ? "true" : "false");
    }
    if (const auto* n = dyn_cast<StringLiteral>(node)) {
        return "StringLiteral \"" + n->value() + "\"";
    }
    if (const auto* n = dyn_cast<Identifier>(node)) {
        return "Identifier " + n->name();
    }
    if (const auto* n = dyn_cast<UnaryExpr>(node)) {
        return "UnaryExpr '" + n->op() + "'";
    }
    if (const auto* n = dyn_cast<BinaryExpr>(node)) {
        return "BinaryExpr '" + n->op() + "'";
    }
    if (isa<Assign>(node)) {
        return "Assign";
    }
    if (isa<ExprStmt>(node)) {
        return "ExprStmt";
    }
    if (isa<IfStmt>(node)) {
        return "IfStmt";
    }
    if (isa<ContinueStmt>(node)) {
        return "ContinueStmt";
    }
    if (isa<Block>(node)) {
        return "Block";
    }
    if (const auto* n = dyn_cast<VarDecl>(node)) {
        return "VarDecl '" + n->name() + "'";
    }
    if (isa<TranslationUnit>(node)) {
        return "TranslationUnit";
    }
    return "<unknown>";
}

// An edge to a child, with an optional role label ("lhs: ", "type: ", ...).
using Child = std::pair<std::string, const ASTNode*>;

std::vector<Child> node_children(const ASTNode* node) {
    std::vector<Child> children;
    if (const auto* n = dyn_cast<UnaryExpr>(node)) {
        children.push_back({"", n->operand()});
    } else if (const auto* n = dyn_cast<BinaryExpr>(node)) {
        children.push_back({"lhs: ", n->lhs()});
        children.push_back({"rhs: ", n->rhs()});
    } else if (const auto* n = dyn_cast<Assign>(node)) {
        children.push_back({"target: ", n->target()});
        children.push_back({"value: ", n->value()});
    } else if (const auto* n = dyn_cast<ExprStmt>(node)) {
        children.push_back({"", n->expr()});
    } else if (const auto* n = dyn_cast<IfStmt>(node)) {
        children.push_back({"cond: ", n->cond()});
        children.push_back({"then: ", n->then_branch()});
        if (n->else_branch() != nullptr) {
            children.push_back({"else: ", n->else_branch()});
        }
    } else if (const auto* n = dyn_cast<Block>(node)) {
        for (const auto& stmt : n->stmts()) {
            children.push_back({"", stmt.get()});
        }
    } else if (const auto* n = dyn_cast<VarDecl>(node)) {
        if (n->type() != nullptr) {
            children.push_back({"type: ", n->type()});
        }
        if (n->init() != nullptr) {
            children.push_back({"init: ", n->init()});
        }
    } else if (const auto* n = dyn_cast<TranslationUnit>(node)) {
        for (const auto& stmt : n->stmts()) {
            children.push_back({"", stmt.get()});
        }
    }
    return children;
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

std::string dump_tree(const ASTNode* node) {
    std::string out = node_label(node) + "\n";
    render(node, "", out);
    return out;
}

} // namespace blast::core::parser
