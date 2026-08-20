#include <core/context/AstContext.hpp>

namespace blast::core::context {

AstContext::AstContext():
        m_core(Scope::Kind::Module),
        m_main(Scope::Kind::Module),
        m_node_types(),
        m_node_symbols(),
        m_types()
    {
        // Declate builtins

        // 64-bits signed integer
        auto int_type = std::make_unique<PrimitiveType>("Int", nullptr, this->m_types.size(), 64);
        auto int_symbol = this->m_core.declare(Symbol::Kind::Type, "Int");
        int_symbol->setType(int_type.get());
        this->m_types.push_back(std::move(int_type));
        
        // Make 'main' (the unit to be analysed) import all Core symbols by default
        this->m_main.addGlobImport(&m_core);
    }

void AstContext::reserveNodes(parser::NodeId count) {
    if (count > this->m_node_types.size()) {
        this->m_node_types.resize(count, nullptr);
        this->m_node_symbols.resize(count, nullptr);
    }
}

void AstContext::setNodeSymbol(parser::NodeId id, Symbol* symbol) {
    if (id == parser::INVALID_NODE_ID) {
        return;
    }
    this->reserveNodes(id + 1);
    this->m_node_symbols[id] = symbol;
}

Symbol* AstContext::nodeSymbol(parser::NodeId id) {
    return id < this->m_node_symbols.size() ? this->m_node_symbols[id] : nullptr;
}

void AstContext::setNodeType(parser::NodeId id, const Type* type) {
    if (id == parser::INVALID_NODE_ID) {
        return;
    }
    this->reserveNodes(id + 1);
    this->m_node_types[id] = type;
}

const Type* AstContext::nodeType(parser::NodeId id) const {
    return id < this->m_node_types.size() ? this->m_node_types[id] : nullptr;
}

} // namespace blast::core::context
