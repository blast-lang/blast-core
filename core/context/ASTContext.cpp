#include <core/context/ASTContext.hpp>

namespace blast::core::context {

ASTContext::ASTContext():
        m_core(Scope::Kind::Module, 0),
        m_main(Scope::Kind::Module, 1),
        m_scopes(),
        m_next_scope_id(2),
        m_next_symbol_id(0),
        m_types(),
        m_node_types(),
        m_node_symbls()
    {

        //---------------------------
        // Builtin Types
        //---------------------------
        // 64-bits signed integer
        auto INT_T = this->registerType(std::make_unique<PrimitiveType>("Int", nullptr, 64));
        auto INT_S = this->declare(this->m_core, "Int", Symbol::Kind::Type);
        INT_S->setType(INT_T);

        // Fieldless struct, the type of 'nothing'
        auto NOTHING_T = this->registerType(std::make_unique<StructType>("Nothing", nullptr, std::vector<StructType::Field>{}));
        auto NOTHING_S = this->declare(this->m_core, "Nothing", Symbol::Kind::Type);
        NOTHING_S->setType(NOTHING_T);

        //---------------------------
        // Builtin Functions
        //---------------------------
        auto PRINT_ARGS_T = this->getTuple({INT_T});
        auto PRINT_T = this->getFunction(PRINT_ARGS_T, NOTHING_T);
        auto PRINT_S = this->declare(this->m_core, "print", Symbol::Kind::Function);
        PRINT_S->setType(PRINT_T);


        // Make 'main' (the unit to be analysed) import all Core symbols by default
        this->m_main.setParent(&this->m_core);
    }

const TupleType* ASTContext::getTuple(std::vector<const Type*> elements) {
    std::vector<TypeId> key;
    key.reserve(elements.size());
    for (const Type* e: elements) {
        key.push_back(e->id());
    }

    // Can we find the tupe? Return it
    auto it = this->m_tuples.find(key);
    if (it != this->m_tuples.end()) {
        return it->second;
    }

    // We can't find it? Build it and register it for future uses
    std::string name = "Tuple{";
    for (std::size_t i = 0; i < elements.size(); ++i) {
        if (i > 0) {
            name += ", ";
        }
        name += elements[i]->name();
    }
    name += "}";

    const TupleType* tuple = this->registerType(std::make_unique<TupleType>(std::move(name), nullptr, std::move(elements)));
    this->m_tuples.emplace(std::move(key), tuple);
    return tuple;
}

FunctionType* ASTContext::getFunction(const TupleType* args, const Type* returns) {
    std::pair<TypeId, TypeId> key{args->id(), returns->id()};
    // Can we find the fct? Return it
    auto it = this->m_functions.find(key);
    if (it != this->m_functions.end()) {
        return it->second;
    }
    // We can't find it? Build it and register it for future uses
    std::string name = "Function{" + args->name() + ", " + returns->name() + "}";
    FunctionType* function = this->registerType(std::make_unique<FunctionType>(std::move(name), nullptr, args, returns));
    this->m_functions.emplace(key, function);
    return function;
}


} // namespace blast::core::context
