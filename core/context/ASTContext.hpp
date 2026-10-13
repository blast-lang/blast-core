#pragma once

#include <core/context/Scope.hpp>
#include <core/parser/Ast.hpp>
#include <vector>
#include <map>
#include <unordered_map>
#include <utility>

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

    Symbol* declare(Scope& scope, const std::string& name, Symbol::Kind kind) {
        return scope.declare(name, kind, this->m_next_symbol_id++);
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

    template <typename T>
    T* registerType(std::unique_ptr<T> type) {
        T* raw = type.get();
        raw->m_id = static_cast<TypeId>(this->m_types.size());
        this->m_types.push_back(std::move(type));
        return raw;
    }

    const TupleType* getTuple(std::vector<const Type*> elements);
    FunctionType* getFunction(const TupleType* args, const Type* returns);

    // Add a function to the program, its id is its position in the list
    FunctionInfo* registerFunction(const std::string& name, FunctionInfo::Linkage linkage, Symbol* symbol) {
        const FunctionId id = static_cast<FunctionId>(this->m_fct_infos.size());
        this->m_fct_infos.push_back(std::make_unique<FunctionInfo>(FunctionInfo{id, name, linkage, symbol}));
        FunctionInfo* fct = this->m_fct_infos.back().get();
        if (symbol) {
            symbol->setFunction(fct);
        }
        return fct;
    }

    const std::vector<std::unique_ptr<FunctionInfo>>& functions() const { return this->m_fct_infos; }

private:
    // Core containing builtins symbols
    Scope m_core;
    // Translation unit to be contextualized
    Scope m_main;
    // Scopes nested under m_main, held by pointer so growing the vector never
    // moves a Scope a Symbol or a child scope already points at
    std::vector<std::unique_ptr<Scope>> m_scopes;
    ScopeId m_next_scope_id;
    SymbolId m_next_symbol_id;
    // Unique set of Types, indexed by TypeId
    std::vector<std::unique_ptr<Type>> m_types;
    // Map Tuple's content to its underlying unique type
    std::map<std::vector<TypeId>, const TupleType*> m_tuples;
    // Map a Function's (args, returns) ids to its underlying unique type
    std::map<std::pair<TypeId, TypeId>, FunctionType*> m_functions;
    // Every function of the program, indexed by FunctionId. Held by pointer so
    // growing the vector never moves a record a Symbol points at
    std::vector<std::unique_ptr<FunctionInfo>> m_fct_infos;
    
    // Map nodes to their deduced types
    std::unordered_map<parser::NodeId, Type*> m_node_types;
    // Map nodes to their deduced symbols
    std::unordered_map<parser::NodeId, Symbol*> m_node_symbls;
};

} // namespace blast::core::context
