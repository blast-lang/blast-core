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
        auto INT_T = this->registerType(std::make_unique<PrimitiveType>("Int64", nullptr, 64, true));
        auto INT_S = this->declare(this->m_core, "Int64", Symbol::Kind::Type);
        INT_S->setType(INT_T);

        // 32-bits signed integer
        auto INT32_T = this->registerType(std::make_unique<PrimitiveType>("Int32", nullptr, 32, true));
        auto INT32_S = this->declare(this->m_core, "Int32", Symbol::Kind::Type);
        INT32_S->setType(INT32_T);

        auto INT16_T = this->registerType(std::make_unique<PrimitiveType>("Int16", nullptr, 16, true));
        auto INT16_S = this->declare(this->m_core, "Int16", Symbol::Kind::Type);
        INT16_S->setType(INT16_T);

        auto INT8_T = this->registerType(std::make_unique<PrimitiveType>("Int8", nullptr, 8, true));
        auto INT8_S = this->declare(this->m_core, "Int8", Symbol::Kind::Type);
        INT8_S->setType(INT8_T);

        // Unsigned integers
        auto UINT64_T = this->registerType(std::make_unique<PrimitiveType>("UInt64", nullptr, 64, false));
        auto UINT64_S = this->declare(this->m_core, "UInt64", Symbol::Kind::Type);
        UINT64_S->setType(UINT64_T);

        auto UINT32_T = this->registerType(std::make_unique<PrimitiveType>("UInt32", nullptr, 32, false));
        auto UINT32_S = this->declare(this->m_core, "UInt32", Symbol::Kind::Type);
        UINT32_S->setType(UINT32_T);

        auto UINT16_T = this->registerType(std::make_unique<PrimitiveType>("UInt16", nullptr, 16, false));
        auto UINT16_S = this->declare(this->m_core, "UInt16", Symbol::Kind::Type);
        UINT16_S->setType(UINT16_T);

        auto UINT8_T = this->registerType(std::make_unique<PrimitiveType>("UInt8", nullptr, 8, false));
        auto UINT8_S = this->declare(this->m_core, "UInt8", Symbol::Kind::Type);
        UINT8_S->setType(UINT8_T);

        // Truth value, stored in a byte
        auto BOOL_T = this->registerType(std::make_unique<PrimitiveType>("Bool", nullptr, 8, false));
        auto BOOL_S = this->declare(this->m_core, "Bool", Symbol::Kind::Type);
        BOOL_S->setType(BOOL_T);

        // Unicode code point, Julia-style
        auto CHAR_T = this->registerType(std::make_unique<PrimitiveType>("Char", nullptr, 32, false));
        auto CHAR_S = this->declare(this->m_core, "Char", Symbol::Kind::Type);
        CHAR_S->setType(CHAR_T);

        // Fieldless struct, the type of 'nothing'
        auto NOTHING_T = this->registerType(std::make_unique<StructType>("Nothing", nullptr, std::vector<StructType::Field>{}));
        auto NOTHING_S = this->declare(this->m_core, "Nothing", Symbol::Kind::Type);
        NOTHING_S->setType(NOTHING_T);

        //---------------------------
        // Builtin Functions
        //---------------------------
        // The unit's top-level code, always function 0
        GenericFunction* MAIN_G = this->registerGeneric("blast_main", nullptr);
        this->registerMethod(MAIN_G, Method::Linkage::Internal, nullptr, nullptr);

        auto PRINT_ARGS_T = this->getTuple({INT_T});
        auto PRINT_T = this->getFunction(PRINT_ARGS_T, NOTHING_T);
        auto PRINT_S = this->declare(this->m_core, "print", Symbol::Kind::Function);
        PRINT_S->setType(PRINT_T);
        GenericFunction* PRINT_G = this->registerGeneric("print", PRINT_S);
        this->registerMethod(PRINT_G, Method::Linkage::External, PRINT_T, nullptr);


        //---------------------------
        // Builtin Modules
        //---------------------------
        auto CORE_S = this->declare(this->m_core, "Core", Symbol::Kind::Module);
        CORE_S->setModuleScope(&this->m_core);
        auto MAIN_S = this->declare(this->m_core, "Main", Symbol::Kind::Module);
        MAIN_S->setModuleScope(&this->m_main);

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
