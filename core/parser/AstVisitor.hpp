#pragma once
#include <core/parser/Ast.hpp>
#include <type_traits>
#include <vector>

namespace blast::core::parser {

template <typename Derived, typename R = void>
class AstVisitor {
    using Kind = ASTNode::Kind;
public:
    // Reached for non-overrided visits.
    R visitEmpty() {
        if constexpr (!std::is_void_v<R>) {
            return R{};
        }
    }

    // To Override: what to do when we visit a node
    R visitIntLiteral(const IntLiteral& n)             { return self().visitEmpty(); }
    R visitFloatLiteral(const FloatLiteral& n)         { return self().visitEmpty(); }
    R visitBoolLiteral(const BoolLiteral& n)           { return self().visitEmpty(); }
    R visitStringLiteral(const StringLiteral& n)       { return self().visitEmpty(); }
    R visitIdentifier(const Identifier& n)             { return self().visitEmpty(); }
    R visitUnaryExpr(const UnaryExpr& n)               { return self().visitEmpty(); }
    R visitBinaryExpr(const BinaryExpr& n)             { return self().visitEmpty(); }
    R visitAssign(const Assign& n)                     { return self().visitEmpty(); }
    R visitExprStmt(const ExprStmt& n)                 { return self().visitEmpty(); }
    R visitIfStmt(const IfStmt& n)                     { return self().visitEmpty(); }
    R visitContinueStmt(const ContinueStmt& n)         { return self().visitEmpty(); }
    R visitBlock(const Block& n)                       { return self().visitEmpty(); }
    R visitVarDecl(const VarDecl& n)                   { return self().visitEmpty(); }
    R visitTranslationUnit(const TranslationUnit& n)   { return self().visitEmpty(); }
    // Non-concrete
    R visitExpr(const Expr& n)                         { return self().visitNode(n); }
    R visitDecl(const Decl& n)                         { return self().visitStmt(n); }
    R visitStmt(const Stmt& n)                         { return self().visitNode(n); }
    // Default
    R visitNode(const ASTNode&) { return self().visitEmpty(); }

    // Entry point: dispatch `node` to the matching handler on Derived.
    R visit(const ASTNode* node) {
        if (node == nullptr) {
            return self().visitEmpty();
        }
        switch (node->kind()) {
            // Literals
            case Kind::IntLiteral:
                return self().visitIntLiteral(as<IntLiteral>(node));
            case Kind::FloatLiteral:
                return self().visitFloatLiteral(as<FloatLiteral>(node));
            case Kind::BoolLiteral:
                return self().visitBoolLiteral(as<BoolLiteral>(node));
            case Kind::StringLiteral:
                return self().visitStringLiteral(as<StringLiteral>(node));
            // Expressions
            case Kind::Identifier:
                return self().visitIdentifier(as<Identifier>(node));
            case Kind::UnaryExpr:
                return self().visitUnaryExpr(as<UnaryExpr>(node));
            case Kind::BinaryExpr:
                return self().visitBinaryExpr(as<BinaryExpr>(node));
            case Kind::Assign:
                return self().visitAssign(as<Assign>(node));
            // Statements
            case Kind::ExprStmt:
                return self().visitExprStmt(as<ExprStmt>(node));
            case Kind::IfStmt:
                return self().visitIfStmt(as<IfStmt>(node));
            case Kind::ContinueStmt:
                return self().visitContinueStmt(as<ContinueStmt>(node));
            case Kind::Block:
                return self().visitBlock(as<Block>(node));
            // Declarations
            case Kind::VarDecl:
                return self().visitVarDecl(as<VarDecl>(node));
            // Root
            case Kind::TranslationUnit:
                return self().visitTranslationUnit(as<TranslationUnit>(node));
        }
        // Only reachable if a Kind was added without a case above.
        return self().visitEmpty();
    }

private:
    Derived& self() { return static_cast<Derived&>(*this); }
    // The Kind tag already proved the type; no need to pay for a check.
    template <typename T>
    static const T& as(const ASTNode* node) { return *static_cast<const T*>(node); }
};

} // namespace blast::core::parser
