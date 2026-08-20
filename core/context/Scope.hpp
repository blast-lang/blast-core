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
    Symbol(Kind kind, std::string name, Scope* scope, const parser::Decl* decl):
        m_kind(kind), m_name(std::move(name)), m_scope(scope), m_decl(decl), m_type(nullptr),
        m_module_scope(nullptr), m_id(INVALID_SYMBOL_ID) {}

    // Owned by the scope that declared it, and compared by address.
    Symbol(const Symbol&) = delete;
    Symbol& operator=(const Symbol&) = delete;

    Kind kind() const { return this->m_kind; }
    const std::string& name() const { return this->m_name; }
    Scope* scope() const { return this->m_scope; }

    // Where the name was introduced. Null for builtins, which no source declares.
    const parser::Decl* decl() const { return this->m_decl; }

    // Null until the type checker fills it in.
    const Type* type() const { return this->m_type; }
    void setType(const Type* type) { this->m_type = type; }

    // The scope this name stands for; null unless kind() is Kind::Module.
    // Qualified lookup crosses from a name to a scope through here.
    Scope* moduleScope() const { return this->m_module_scope; }
    void setModuleScope(Scope* scope) { this->m_module_scope = scope; }

    // Index of this binding within its function frame. Handed out by the
    // resolver so the IR can address a local by id rather than by name.
    SymbolId id() const { return this->m_id; }
    void setId(SymbolId id) { this->m_id = id; }
    bool hasId() const { return this->m_id != INVALID_SYMBOL_ID; }

private:
    Kind m_kind;
    std::string m_name;
    Scope* m_scope;
    const parser::Decl* m_decl;
    const Type* m_type;
    Scope* m_module_scope;
    SymbolId m_id;
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

    // What lookup() found. A name supplied by two glob imports at once is not
    // an error where the `using` was written, only where the name is used, so
    // the ambiguity is reported back rather than silently resolved.
    struct LookupResult {
        Symbol* symbol = nullptr;   // null when not found, and when ambiguous
        bool ambiguous = false;

        explicit operator bool() const { return this->symbol != nullptr; }
    };

private:
    // Lets lookup() take a string_view without materialising a std::string.
    struct NameHash {
        using is_transparent = void;
        std::size_t operator()(std::string_view name) const {
            return std::hash<std::string_view>{}(name);
        }
    };

public:
    explicit Scope(Kind kind, Scope* parent = nullptr): m_kind(kind), m_parent(parent) {}

    // Owns its symbols and its children; both are referred to by raw pointer
    // from elsewhere, so it must not move or copy.
    Scope(const Scope&) = delete;
    Scope& operator=(const Scope&) = delete;

    Kind kind() const { return this->m_kind; }
    Scope* parent() const { return this->m_parent; }

    Scope* createChild(Kind kind) {
        this->m_children.push_back(std::make_unique<Scope>(kind, this));
        return this->m_children.back().get();
    }

    // Create a module nested in this scope: its scope, the symbol that names it
    // here, and the implicit `using Core` every module gets. Returns null if
    // the name is already taken.
    //
    // The implicit import lives here rather than at the call sites so it cannot
    // be forgotten. Core is the one module built without it -- it is created
    // directly by the context, with `core` null, since it cannot import itself.
    Symbol* createModule(std::string name, Scope* core, const parser::Decl* decl = nullptr) {
        Symbol* symbol = this->declare(Symbol::Kind::Module, std::move(name), decl);
        if (symbol == nullptr) {
            return nullptr;
        }
        Scope* scope = this->createChild(Kind::Module);
        symbol->setModuleScope(scope);
        if (core != nullptr) {
            scope->addGlobImport(core);
        }
        return symbol;
    }

    // Bring everything a module declares into view unqualified, as `using` does.
    // Not transitive: what `imported` itself imported does not come along, which
    // is why the search below is lookupLocal and not lookup.
    void addGlobImport(Scope* imported) { this->m_glob_imports.push_back(imported); }

    // Bind `name` here. Returns null if this scope already binds it: shadowing
    // an outer scope is legal, redeclaring in the same one is not, and that is
    // the distinction the caller reports on.
    Symbol* declare(Symbol::Kind kind, std::string name, const parser::Decl* decl = nullptr) {
        if (this->m_symbols.contains(name)) {
            return nullptr;
        }
        auto symbol = std::make_unique<Symbol>(kind, name, this, decl);
        Symbol* raw = symbol.get();
        
        this->m_symbols.emplace(std::move(name), std::move(symbol));
        this->m_order.push_back(raw);
        return raw;
    }

    // This scope only what a redeclaration check needs.
    Symbol* lookupLocal(std::string_view name) const {
        const auto it = this->m_symbols.find(name);
        return it == this->m_symbols.end() ? nullptr : it->second.get();
    }

    // The modules brought in by `using`, this scope only. Core reaches every
    // module through here, so there is no separate prelude path.
    LookupResult lookupGlob(std::string_view name) const {
        LookupResult result;
        for (const Scope* imported : this->m_glob_imports) {
            Symbol* found = imported->lookupLocal(name);
            if (found == nullptr) {
                continue;
            }
            if (result.symbol != nullptr && result.symbol != found) {
                return {nullptr, true};
            }
            result.symbol = found;
        }
        return result;
    }

    // Walk outward through the enclosing scopes; first hit wins. Within one
    // scope the order is declarations first, glob imports second. A name
    // written here beats one a `using` brought in, which is what makes a local
    // Int shadow Core.Int rather than collide with it.
    LookupResult lookup(std::string_view name) const {
        for (const Scope* scope = this; scope != nullptr; scope = scope->m_parent) {
            if (Symbol* found = scope->lookupLocal(name)) {
                return {found, false};
            }
            const LookupResult imported = scope->lookupGlob(name);
            if (imported.symbol != nullptr || imported.ambiguous) {
                return imported;
            }
            // Searched this scope. A module is a lookup root: code inside one
            // does not see the bindings of the module that encloses it, so the
            // only way in from outside is qualification or an import.
            if (scope->m_kind == Kind::Module) {
                break;
            }
        }
        return {};
    }

    // Symbols of this scope in declaration order. The map alone would iterate
    // in an unspecified order, and a compiler wants its output deterministic.
    const std::vector<Symbol*>& symbols() const { return this->m_order; }
    const std::vector<std::unique_ptr<Scope>>& children() const { return this->m_children; }

private:
    Kind m_kind;
    Scope* m_parent;
    std::vector<std::unique_ptr<Scope>> m_children;
    // Modules made visible unqualified by `using`; Core is the implicit first
    // entry for every module. Not owned, an imported module outlives the
    // importer, and importing does not nest.
    std::vector<Scope*> m_glob_imports;
    std::unordered_map<std::string, std::unique_ptr<Symbol>, NameHash, std::equal_to<>> m_symbols;
    std::vector<Symbol*> m_order;
};

} // namespace blast::core::context