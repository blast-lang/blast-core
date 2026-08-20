#include <core/context/Resolver.hpp>
#include <cstdio>

namespace blast::core::context {


void ScopeResolver::run(const parser::TranslationUnit& unit) {
    this->m_current_scope = &this->m_ctx->mainScope();
    this->visit(&unit);
}


void ScopeResolver::visitTranslationUnit(const parser::TranslationUnit& node) {
    for (const auto& stmt : node.stmts()) {
        this->visit(stmt.get());
    }
}

void ScopeResolver::visitExprStmt(const parser::ExprStmt& node) {
    this->visit(node.expr());
}

void ScopeResolver::visitVarDecl(const parser::VarDecl& node) {
    // Resolve the type annotation, it is a name use like any other
    if (node.hasType() && node.type()->kind() == parser::ASTNode::Kind::Identifier) {
        const auto* type_name = static_cast<const parser::Identifier*>(node.type());
        const Scope::LookupResult r = this->m_current_scope->lookup(type_name->name());
        // Register the type Symbol
        this->m_ctx->setNodeSymbol(type_name->id(), r.symbol);
    }

    // Try declaring variable into the current scope
    Symbol* v = this->m_current_scope->declare(Symbol::Kind::Variable, node.name(), &node);

    if (v != nullptr) {
        // Set new id of Symbol as this is a genuine new one
        v->setId(this->m_next_id++);
        // Store it on the node's symbol table
        this->m_ctx->setNodeSymbol(node.id(), v);
    } else {
        // Throw 'already declared in this scope'
    }

    std::printf("Found declaration of %s in scope %d \n", node.name().c_str(),
                static_cast<int>(this->m_current_scope->kind()));
}

void TypeResolver::run(const parser::TranslationUnit& unit) {
    this->visit(&unit);
}


const Type* TypeResolver::visitTranslationUnit(const parser::TranslationUnit& node) {
    for (const auto& stmt : node.stmts()) {
        this->visit(stmt.get());
    }
    return nullptr;
}

const Type* TypeResolver::visitExprStmt(const parser::ExprStmt& node) {
    return this->visit(node.expr());
}

const Type* TypeResolver::visitVarDecl(const parser::VarDecl& node) {
    // At this point, scope and symbol where already resolved
    Symbol* v = this->m_ctx->nodeSymbol(node.id());
    // Get this variable's declaration type by using the NodeId of its Expr node
    Symbol* v_type_symbol = this->m_ctx->nodeSymbol(node.type()->id());
    const Type* v_type = v_type_symbol ? v_type_symbol->type() : nullptr;

    if (v == nullptr) {
        // throw unkown variable
    }

   if (node.init() != nullptr) {
        //const Type* actual = this->visit(node.init());
        // throw mismatch -> type error
    }

    // Set this variable's type info
    v->setType(v_type);
    // Set this node's type info
    this->m_ctx->setNodeType(node.id(), v_type);

    std::printf("Variable %s is of type %s \n", v->name().c_str(),
                v_type ? v_type->name().c_str() : "<unresolved>");
    return v_type;
}

}