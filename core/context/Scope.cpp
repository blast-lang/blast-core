#include <core/context/Scope.hpp>

namespace blast::core::context {

bool Scope::hasSymbol(std::string_view name) const {
    return this->m_symbol_index.contains(name);
}

Symbol* Scope::declare(const std::string& name, Symbol::Kind kind) {
    if (this->hasSymbol(name)) {
        // Throw "Symbol already Declared in this Scope"
        return nullptr;
    }
    std::unique_ptr<Symbol> new_symbl = std::make_unique<Symbol>(kind, this->m_symbols.size(), name, this);
    Symbol* new_symbl_ptr = new_symbl.get();
    // Add new symbol declaration in order
    this->m_symbols.push_back(std::move(new_symbl));
    // Register it by its name
    this->m_symbol_index.emplace(new_symbl_ptr->name(), new_symbl_ptr);
    return new_symbl_ptr;
}

Symbol* Scope::lookup(std::string_view name) const {
    auto it = this->m_symbol_index.find(name);
    if (it != this->m_symbol_index.end()) {
        return it->second;
    }

    // Otherwise, glimb up the scope structure to find symbol
    if (this->m_parent) {
        return this->m_parent->lookup(name);
    }

    return nullptr;
}

}
