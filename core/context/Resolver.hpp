#pragma once

#include <core/context/Type.hpp>
#include <core/parser/AstVisitor.hpp>
#include <core/context/Scope.hpp>
#include <core/context/ASTContext.hpp>

namespace blast::core::context {

class ScopeResolver: public parser::AstVisitor<ScopeResolver, void> {
public:
    ScopeResolver(ASTContext& ctx): m_ctx(&ctx), m_current_scope(nullptr) {}
private:
    ASTContext* m_ctx;
    Scope*      m_current_scope;

public:
    void run(const parser::TranslationUnit& unit);
    void visitVarDecl(const parser::VarDecl& node);
    void visitIdentifier(const parser::Identifier& node);
    void visitBinaryExpr(const parser::BinaryExpr& node);
    void visitAssign(const parser::Assign& node);
    void visitExprStmt(const parser::ExprStmt& node);
    void visitTranslationUnit(const parser::TranslationUnit& node);
};

// Fills Symbol::setType() and ASTContext::setNodeType()
class TypeResolver: public parser::AstVisitor<TypeResolver, const Type*> {
public:
    TypeResolver(ASTContext& ctx): m_ctx(&ctx) {}
private:
    ASTContext* m_ctx;

public:
    void run(const parser::TranslationUnit& unit);
    const Type* visitVarDecl(const parser::VarDecl& node);
    const Type* visitIdentifier(const parser::Identifier& node);
    const Type* visitBinaryExpr(const parser::BinaryExpr& node);
    const Type* visitAssign(const parser::Assign& node);
    const Type* visitExprStmt(const parser::ExprStmt& node);
    const Type* visitTranslationUnit(const parser::TranslationUnit& node);
};

} // namespace blast::core::context