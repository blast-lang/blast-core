#pragma once
#include <core/context/Type.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace blast::core::parser {
class Decl;
}

namespace blast::core::context {
class Scope;

// Identity of a binding within its function frame. The id of a symbol the
// resolver has not placed in a frame yet.
using SymbolId = std::uint32_t;
inline constexpr SymbolId INVALID_SYMBOL_ID = static_cast<SymbolId>(-1);


// Identity of a binding within its function frame. The id of a symbol the
// resolver has not placed in a frame yet.
using ScopeId = std::uint32_t;
inline constexpr SymbolId INVALID_SCOPE_ID = static_cast<ScopeId>(-1);


// The identity of one declaration.
//
// Name resolution happens exactly once; after it every phase refers to the
// Symbol and never to the identifier string again. Two names denote the same
// binding iff they resolve to the same Symbol address.
// BLAST follows Julia in having a single namespace: a type name is a binding
// like any other, one whose symbol has Kind::Type. So a scope maps name ->
// symbol, with no (namespace, name) key to disambiguate.
class Symbol {
public:
    enum class Kind {
        Variable,   // a local or global binding
        Parameter,  // a formal of the enclosing function
        Function,   // a generic function: the name owns the method set, and
                    // dispatch not name resolution picks the method
        Type,       // a name bound to a Type
        Module,     // a name bound to a Scope
        Alias       // a name introduced by a selective import
    };

public:
    Symbol(const Symbol&) = delete;
    Symbol& operator=(const Symbol&) = delete;
    Symbol(Kind kind, SymbolId id, std::string name, Scope* parent = nullptr): 
        m_kind(kind), m_id(id), m_name(std::move(name)), m_parent_scope(parent), m_type(nullptr)
    {}

    Kind kind() const { return this->m_kind; }
    SymbolId id() const { return this->m_id; }
    std::string_view name() const { return this->m_name; }

    Type* type() { return this->m_type; }
    void setType(Type* type) { this->m_type = type; }
    
private:
    // Type of Symbol
    Kind m_kind;
    // uuid
    SymbolId m_id;
    // Human readble name of symbol
    std::string m_name;
    // Scope it belongs to
    Scope* m_parent_scope;
    // Deduced type of Symbol, deduced from static analysis
    Type* m_type;
};



// One level of the lexical scope tree: a name -> Symbol map plus a link to the
// enclosing level. lookup() walks outward and stops at the first hit.
class Scope {
public:
    enum class Kind {
        Module,     // a module body, Core and the translation unit's own included
        Function,   // a function body; symbol ids are numbered from here
        Block,      // any nested block
    };

public:
    Scope(Kind kind, ScopeId id = 0, Scope* parent = nullptr, Symbol* name = nullptr): 
        m_kind(kind), m_id(id), m_parent(parent), m_name(name), m_symbols(), m_symbol_index() 
    {}

    bool hasSymbol(std::string_view name) const;
    Symbol* declare(const std::string& name, Symbol::Kind kind);
    // Find a symbol in this scope, or parent scopes
    Symbol* lookup(std::string_view name) const;

    Kind kind() const { return this->m_kind; }
    ScopeId id() const { return this->m_id; }

    Scope* parent() { return this->m_parent; }
    bool hasParent() const { return this->m_parent != nullptr; }
    void setParent(Scope* parent) { this->m_parent = parent; }

private:
    Kind m_kind;
    // uuid
    ScopeId m_id;
    // Parent Scope, Scope that created this scope
    Scope* m_parent;
    // Optionnal Symbol id for named scope (module, function)
    Symbol* m_name;
    // List of unique symbols declared in this scope (in order of declaration)
    // Indexed by Symbol Id
    std::vector<std::unique_ptr<Symbol>> m_symbols;
    // Map symbol to their names for easy lookup
    std::unordered_map<std::string_view, Symbol*> m_symbol_index;
    
};

} // namespace blast::core::context