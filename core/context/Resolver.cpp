#include <core/context/Resolver.hpp>
#include <core/Exception.hpp>
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
        if (r.ambiguous) {
            throw SemanticError("type name '" + type_name->name() + "' is supplied by more than one import");
        }
        if (r.symbol == nullptr) {
            throw SemanticError("unknown type '" + type_name->name() + "'");
        }
        if (r.symbol->kind() != Symbol::Kind::Type) {
            throw SemanticError("'" + type_name->name() + "' is not a type");
        }
        // Register the type Symbol
        this->m_ctx->setNodeSymbol(type_name->id(), r.symbol);
    }

    // Try declaring variable into the current scope
    Symbol* v = this->m_current_scope->declare(Symbol::Kind::Variable, node.name(), &node);
    if (v == nullptr) {
        throw SemanticError("'" + node.name() + "' is already declared in this scope");
    }

    // Set new id of Symbol as this is a genuine new one
    v->setId(this->m_next_id++);
    // Store it on the node's symbol table
    this->m_ctx->setNodeSymbol(node.id(), v);

    std::printf("Found declaration of %s in scope %d \n", node.name().c_str(),
                static_cast<int>(this->m_current_scope->kind()));
}

void ScopeResolver::visitIdentifier(const parser::Identifier& node) {
    const Scope::LookupResult r = this->m_current_scope->lookup(node.name());
    if (r.ambiguous) {
        throw SemanticError("'" + node.name() + "' is supplied by more than one import");
    }
    if (r.symbol == nullptr) {
        throw SemanticError("'" + node.name() + "' is not declared in this scope");
    }
    this->m_ctx->setNodeSymbol(node.id(), r.symbol);
}

void ScopeResolver::visitBinaryExpr(const parser::BinaryExpr& node) {
    this->visit(node.lhs());
    this->visit(node.rhs());
}

void ScopeResolver::visitAssign(const parser::Assign& node) {
    // Value first, so 'c = c' cannot see the binding the target is about to make
    this->visit(node.value());

    const parser::Expr* target = node.target();
    if (target->kind() != parser::ASTNode::Kind::Identifier) {
        throw SemanticError("assignment target is not a name");
    }
    const auto* name = static_cast<const parser::Identifier*>(target);

    const Scope::LookupResult r = this->m_current_scope->lookup(name->name());
    if (r.ambiguous) {
        throw SemanticError("'" + name->name() + "' is supplied by more than one import");
    }
    if (r.symbol == nullptr) {
        throw SemanticError("undefined name '" + name->name() + "'");
    }
    this->m_ctx->setNodeSymbol(name->id(), r.symbol);
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
    if (v == nullptr) {
        throw SemanticError("'" + node.name() + "' was never resolved to a symbol");
    }

    // Get this variable's declaration type by using the NodeId of its Expr node
    Symbol* v_type_symbol = this->m_ctx->nodeSymbol(node.type()->id());
    if (v_type_symbol == nullptr || v_type_symbol->type() == nullptr) {
        throw SemanticError("no type is bound to the annotation of '" + node.name() + "'");
    }
    const Type* v_type = v_type_symbol->type();

   if (node.init() != nullptr) {
        //const Type* actual = this->visit(node.init());
        // throw mismatch -> type error
    }

    // Set this variable's type info
    v->setType(v_type);
    // Set this node's type info
    this->m_ctx->setNodeType(node.id(), v_type);

    std::printf("Variable %s is of type %s \n", v->name().c_str(), v_type->name().c_str());
    return v_type;
}

}