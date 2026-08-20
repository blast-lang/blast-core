#pragma once

#include <core/context/Type.hpp>
#include <core/parser/AstVisitor.hpp>
#include <core/context/Scope.hpp>
#include <core/context/AstContext.hpp>

namespace blast::core::context {

class ScopeResolver: public parser::AstVisitor<ScopeResolver, void> {
public:
    ScopeResolver(AstContext& ctx): m_ctx(&ctx), m_current_scope(nullptr), m_next_id(0) {}
private:
    AstContext* m_ctx;
    Scope*      m_current_scope;
    SymbolId    m_next_id;

public:
    void run(const parser::TranslationUnit& unit);
    void visitVarDecl(const parser::VarDecl& node);
    void visitExprStmt(const parser::ExprStmt& node);
    void visitTranslationUnit(const parser::TranslationUnit& node);
};

// Fills Symbol::setType() and AstContext::setNodeType()
class TypeResolver: public parser::AstVisitor<TypeResolver, const Type*> {
public:
    TypeResolver(AstContext& ctx): m_ctx(&ctx) {}
private:
    AstContext* m_ctx;

public:
    void run(const parser::TranslationUnit& unit);
    const Type* visitVarDecl(const parser::VarDecl& node);
    const Type* visitExprStmt(const parser::ExprStmt& node);
    const Type* visitTranslationUnit(const parser::TranslationUnit& node);
};

} // namespace blast::core::context