#include <core/context/ASTContext.hpp>

namespace blast::core::context {

ASTContext::ASTContext():
        m_core(Scope::Kind::Module, 0),
        m_main(Scope::Kind::Module, 1),
        m_scopes(),
        m_next_scope_id(2),
        m_types(),
        m_node_types(),
        m_node_symbls()
    {

        //---------------------------
        // Builtin Types
        //---------------------------
        // 64-bits signed integer
        auto INT_T = std::make_unique<PrimitiveType>("Int", nullptr, 0, 64);
        auto INT_S = this->m_core.declare("Int", Symbol::Kind::Type);
        INT_S->setType(INT_T.get());
        
        //---------------------------
        // Builtin Functions
        //---------------------------
        this->m_core.declare("print", Symbol::Kind::Function);
        

        this->m_types.push_back(std::move(INT_T));

        // Make 'main' (the unit to be analysed) import all Core symbols by default
        this->m_main.setParent(&this->m_core);
    }


} // namespace blast::core::context
