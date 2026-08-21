#include <core/context/ASTContext.hpp>

namespace blast::core::context {

ASTContext::ASTContext():
        m_core(Scope::Kind::Module),
        m_main(Scope::Kind::Module),
        m_types(),
        m_node_types(),
        m_node_symbls()
    {
        // Declate builtins

        // 64-bits signed integer
        auto int_type = std::make_unique<PrimitiveType>("Int", nullptr, this->m_types.size(), 64);
        auto int_symbol = this->m_core.declare("Int", Symbol::Kind::Type);
        int_symbol->setType(int_type.get());
        this->m_types.push_back(std::move(int_type));
        
        // Make 'main' (the unit to be analysed) import all Core symbols by default
        this->m_main.setParent(&this->m_core);
    }


} // namespace blast::core::context
