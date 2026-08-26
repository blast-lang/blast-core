#include <core/context/Resolver.hpp>
#include <core/Exception.hpp>
#include <cstdio>

namespace blast::core::context {


void ScopeResolver::run(const parser::TranslationUnit& unit) {
    this->m_current_scope = &this->m_ctx->mainScope();
    this->visit(&unit);
}


void ScopeResolver::visitTranslationUnit(const parser::TranslationUnit& node) {
    for (const auto& stmt: node.stmts()) {
        this->visit(stmt.get());
    }
}

void ScopeResolver::visitExprStmt(const parser::ExprStmt& node) {
    this->visit(node.expr());
}

void ScopeResolver::visitVarDecl(const parser::VarDecl& node) {
    // First let's see if we are declaring a unique symbol
    if (this->m_current_scope->hasSymbol(node.name())) {
        // Throw 'Variable already declared in this scope'
    }

    // Let's declare this new symbol in the scope
    Symbol* v = this->m_current_scope->declare(node.name(), Symbol::Kind::Variable);
    // Let's bind this node to its resolved symbol
    this->m_ctx->setNodeSymbol(&node, v);
    
    if (node.hasType()) {
        this->visit(node.type());
    }

    if (node.hasInit()) {
        this->visit(node.init());
    }
}

void ScopeResolver::visitIdentifier(const parser::Identifier& node) {
    Symbol* s = this->m_current_scope->lookup(node.name());
    if (s == nullptr) {
        // Throw: Identifier not declared in this scope
        std::printf("[AF] Unknown Identifier %s\n", node.name().c_str());
    } else {
        this->m_ctx->setNodeSymbol(&node, s);
        std::printf("[AF] Visited identifier %s\n", node.name().c_str());
    }
}

void ScopeResolver::visitBinaryExpr(const parser::BinaryExpr& node) {
    this->visit(node.lhs());
    this->visit(node.rhs());
}

void ScopeResolver::visitAssign(const parser::Assign& node) {
    this->visit(node.value());
    this->visit(node.target());
}

void TypeResolver::run(const parser::TranslationUnit& unit) {
    this->visit(&unit);
}

const Type* TypeResolver::visitTranslationUnit(const parser::TranslationUnit& node) {
    for (const auto& stmt: node.stmts()) {
        this->visit(stmt.get());
    }
    return nullptr;
}

const Type* TypeResolver::visitExprStmt(const parser::ExprStmt& node) {
    return this->visit(node.expr());
}

const Type* TypeResolver::visitVarDecl(const parser::VarDecl& node) {
    // We already ScopeResolver, so by this point this node should have a symbol for this node
    //
    Symbol* v = this->m_ctx->getNodeSymbol(&node);
    if (node.hasType()) {
        // Lookup the type hint symbol, it was already visited by ScopeResolver
        Symbol* ts = this->m_ctx->getNodeSymbol(node.type());
        Type* declared = ts ? ts->type() : nullptr;
        // If undeclared -> Throw ?
        if (declared) {
            std::printf("[AF] Identifier %s is of type %s\n", node.name().c_str(), declared->name().c_str());
            // Set node's and variable's type
            this->m_ctx->setNodeType(&node, declared);
            v->setType(declared);
            return declared;
        } else {
            std::printf("[AF] Identifier %s has undefined type\n", node.name().c_str());
        }
    }
    return nullptr;
}

const Type* TypeResolver::visitIdentifier(const parser::Identifier& node) {
    return nullptr;
}

const Type* TypeResolver::visitBinaryExpr(const parser::BinaryExpr& node) {
    return nullptr;
}

const Type* TypeResolver::visitAssign(const parser::Assign& node) {
    return nullptr;
}

}