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
    void visitCallExpr(const parser::CallExpr& node);
    void visitMemberExpr(const parser::MemberExpr& node);
    void visitModuleDecl(const parser::ModuleDecl& node);
    void visitExprStmt(const parser::ExprStmt& node);
    void visitIfStmt(const parser::IfStmt& node);
    void visitWhileStmt(const parser::WhileStmt& node);
    void visitBlock(const parser::Block& node);
    void visitReturnStmt(const parser::ReturnStmt& node);
    void visitFctDecl(const parser::FctDecl& node);
    void visitTranslationUnit(const parser::TranslationUnit& node);
};

// Fills Symbol::setType() and ASTContext::setNodeType()
class TypeResolver: public parser::AstVisitor<TypeResolver, const Type*> {
public:
    TypeResolver(ASTContext& ctx): m_ctx(&ctx) {}
private:
    ASTContext* m_ctx;

    const Type* nothingType() { return this->m_ctx->coreScope().lookupLocal("Nothing")->type(); }
    // Type of two paths of control flow, where Nothing means the path returns nothing.
    // Null if they disagree or if either is untyped.
    const Type* joinTypes(const Type* a, const Type* b);

public:
    void run(const parser::TranslationUnit& unit);
    const Type* visitVarDecl(const parser::VarDecl& node);
    const Type* visitIntLiteral(const parser::IntLiteral& node);
    const Type* visitIdentifier(const parser::Identifier& node);
    const Type* visitBinaryExpr(const parser::BinaryExpr& node);
    const Type* visitAssign(const parser::Assign& node);
    const Type* visitCallExpr(const parser::CallExpr& node);
    const Type* visitMemberExpr(const parser::MemberExpr& node);
    const Type* visitModuleDecl(const parser::ModuleDecl& node);
    const Type* visitExprStmt(const parser::ExprStmt& node);
    const Type* visitIfStmt(const parser::IfStmt& node);
    const Type* visitWhileStmt(const parser::WhileStmt& node);
    const Type* visitBlock(const parser::Block& node);
    const Type* visitReturnStmt(const parser::ReturnStmt& node);
    const Type* visitFctDecl(const parser::FctDecl& node);
    const Type* visitTranslationUnit(const parser::TranslationUnit& node);
};

} // namespace blast::core::context