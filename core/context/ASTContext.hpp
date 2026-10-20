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

    void setNodeType(const parser::ASTNode* node, const Type* type) {
        this->m_node_types.emplace(node->id(), type);
    }

    void setNodeSymbol(const parser::ASTNode* node, Symbol* symbl) {
        this->m_node_symbls.emplace(node->id(), symbl);
    }

    void setNodeMethod(const parser::ASTNode* node, Method* method) {
        this->m_node_methods.emplace(node->id(), method);
    }

    Method* getNodeMethod(const parser::ASTNode* node) {
        auto it = this->m_node_methods.find(node->id());
        return it == this->m_node_methods.end() ? nullptr : it->second;
    }

    const Type* getNodeType(const parser::ASTNode* node) {
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

    // Create a generic function, bound to its symbol when it has one
    GenericFunction* registerGeneric(const std::string& name, Symbol* symbol) {
        this->m_generics.push_back(std::make_unique<GenericFunction>(name));
        GenericFunction* generic = this->m_generics.back().get();
        if (symbol) {
            symbol->setFunction(generic);
        }
        return generic;
    }

    // Add a method to a generic function, its id is its position in the program
    Method* registerMethod(GenericFunction* generic, Method::Linkage linkage, const FunctionType* type, const parser::FctDecl* decl) {
        const MethodId id = static_cast<MethodId>(this->m_methods.size());
        Method* method = generic->addMethod(Method{id, generic->name(), linkage, type, decl});
        this->m_methods.push_back(method);
        return method;
    }

    const std::vector<Method*>& methods() const { return this->m_methods; }

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
    // Every generic function, held by pointer so growing the vector never
    // moves one a Symbol points at. They own the methods.
    std::vector<std::unique_ptr<GenericFunction>> m_generics;
    // Every method of the program, indexed by MethodId (not owning)
    std::vector<Method*> m_methods;
    
    // Map nodes to their deduced types
    std::unordered_map<parser::NodeId, const Type*> m_node_types;
    // Map nodes to their deduced symbols
    std::unordered_map<parser::NodeId, Symbol*> m_node_symbls;
    // Map function declarations to the method they define
    std::unordered_map<parser::NodeId, Method*> m_node_methods;
};

} // namespace blast::core::context
