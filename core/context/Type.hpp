#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace blast::core::context {

class Type {
public:
    enum class Kind {
        Unknown,     // placeholder for "not yet inferred" / "cannot infer"
        Abstract,    // Number, Real, Any dispatch-only, holds no values
        Primitive,   // Int, Float, Bool
        Struct,      // named fields
        Tuple,       // covariant; method signatures (and returns) are tuple types
        Function,    // (Int, Int, ...) -> (Float, ...)
    };

public:
    Type(Kind kind, std::string name, const Type* super, std::size_t id): m_kind(kind), m_name(std::move(name)), m_super(super), m_id(id) {}
    virtual ~Type() = default;

    // Uniqued by the context; copying one would break identity comparison.
    Type(const Type&) = delete;
    Type& operator=(const Type&) = delete;

    Kind kind() const { return this->m_kind; }
    std::size_t id() const { return this->m_id; }
    const Type* super() const { return this->m_super; }      // null only for Any, the root

    bool isUnknown() const { return this->m_kind == Kind::Unknown; }
    // Can a value of this type exist at runtime? Abstract types cannot be
    // instantiated; they exist to be dispatched on.
    bool isAbstract() const { return this->m_kind == Kind::Abstract; }
private:
    Kind m_kind;
    std::string m_name;
    const Type* m_super;
    std::size_t m_id;
};

// ------------------------------------
// Simple Types
// ------------------------------------

class AbstractType: public Type {
public:
    AbstractType(std::string name, const Type* super, std::size_t id): Type(Kind::Abstract, std::move(name), super, id) {}
};

class PrimitiveType: public Type {
public:
    PrimitiveType(std::string name, const Type* super, std::size_t id, std::size_t bitWidth): Type(Kind::Primitive, std::move(name), super, id), m_bit_width(bitWidth) {}
    // The width the declaration asks for, e.g. 64 for Int64. Distinct from
    // the ABI size and alignment the type gets on a given target, which the
    // DataLayout decides.
    std::size_t bitWidth() const { return this->m_bit_width; }
private:
    std::size_t m_bit_width;
};

// ------------------------------------
// Structural Types
// ------------------------------------

// A concrete composite with named fields
class StructType: public Type {
public:
    struct Field {
        std::string name;
        const Type* type = nullptr;
    };

public:
    // Field *order* is part of the declaration, the offset will be done during DataLayout phase
    StructType(std::string name, const Type* super, std::size_t id, std::vector<StructType::Field> fields): Type(Kind::Struct, std::move(name), super, id), m_fields(std::move(fields)) {}
    const std::vector<StructType::Field>& fields() const { return this->m_fields; }
private:
    std::vector<StructType::Field> m_fields;
};

// A Tuple of types is a sequence of types, and the representation of a method signature.
// Tuples are the one covariant construct in the system:
// Tuple{Int, Int} <: Tuple{Number, Number}. That is what makes multiple dispatch work
// This only works because this is an immutable sequence
class TupleType: public Type {
public:
    TupleType(std::string name, const Type* super, std::size_t id, std::vector<const Type*> elements): Type(Kind::Tuple, std::move(name), super, id), m_elements(std::move(elements)) {}
    const std::vector<const Type*>& elements() const { return this->m_elements; }
private:
    std::vector<const Type*> m_elements;
};


// Invariant: two function types are related only if identical.
class FunctionType: public Type {
public:
    FunctionType(std::string name, const Type* super, std::size_t id, const TupleType* args, const Type* returns):
        Type(Kind::Function, std::move(name), super, id), m_args(args), m_returns(returns) {}

    const TupleType* args() const { return this->m_args; }
    const Type* returns() const { return this->m_returns; }

private:
    const TupleType* m_args;
    const Type* m_returns;
};


// a <: b  -- walk a's super chain looking for b
bool isSubtype(const Type* a, const Type* b);


} // namespace blast::core::context
