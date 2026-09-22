#pragma once

#include <core/context/Scope.hpp>
#include <core/parser/Ast.hpp>
#include <vector>
#include <unordered_map>

namespace blast::core::context {
// Everything one translation unit owns beyond its syntax tree: the types, the
// scope tree, and the semantic facts attached to nodes.
class ASTContext {
public:
    ASTContext();
    ~ASTContext() = default;

    // Will own the AST, the types and the scopes everything else points into.
    ASTContext(const ASTContext&) = delete;
    ASTContext& operator=(const ASTContext&) = delete;

    // Where the resolver starts: the translation unit's own module scope.
    Scope& mainScope() { return this->m_main; }
    // The builtins. Every module glob-imports it, so lookups reach it without
    // naming it; this is for the few places that need the scope itself.
    Scope& coreScope() { return this->m_core; }

    // Nested scopes outlive the resolver that opens them: a Scope owns the
    // Symbols declared in it, and nodes keep pointing at those symbols.
    Scope& newScope(Scope::Kind kind, Scope* parent) {
        this->m_scopes.push_back(std::make_unique<Scope>(kind, this->m_next_scope_id++, parent));
        return *this->m_scopes.back();
    }

    void setNodeType(const parser::ASTNode* node, Type* type) {
        this->m_node_types.emplace(node->id(), type);
    }

    void setNodeSymbol(const parser::ASTNode* node, Symbol* symbl) {
        this->m_node_symbls.emplace(node->id(), symbl);
    }

    Type* getNodeType(const parser::ASTNode* node) {
        auto it = this->m_node_types.find(node->id());
        return it == this->m_node_types.end() ? nullptr : it->second;
    }

    Symbol* getNodeSymbol(const parser::ASTNode* node) {
        auto it = this->m_node_symbls.find(node->id());
        return it == this->m_node_symbls.end() ? nullptr : it->second;
    }

private:
    // Core containing builtins symbols
    Scope m_core;
    // Translation unit to be contextualized
    Scope m_main;
    // Scopes nested under m_main, held by pointer so growing the vector never
    // moves a Scope a Symbol or a child scope already points at
    std::vector<std::unique_ptr<Scope>> m_scopes;
    ScopeId m_next_scope_id;
    // Unique set of Types, indexed by TypeId
    std::vector<std::unique_ptr<Type>> m_types;
    // Map nodes to their deduced types
    std::unordered_map<parser::NodeId, Type*> m_node_types;
    // Map nodes to their deduced symbols
    std::unordered_map<parser::NodeId, Symbol*> m_node_symbls;
};

} // namespace blast::core::context
