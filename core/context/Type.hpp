#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace blast::core::context {

// Index of a type in the context's arena: m_types[t->id()].get() == t.
using TypeId = std::uint32_t;
inline constexpr TypeId INVALID_TYPE_ID = static_cast<TypeId>(-1);

class ASTContext;

class Type {
    friend class ASTContext;
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
    Type(Kind kind, std::string name, const Type* super): m_kind(kind), m_name(std::move(name)), m_super(super), m_id(INVALID_TYPE_ID) {}
    virtual ~Type() = default;

    // Uniqued by the context; copying one would break identity comparison.
    Type(const Type&) = delete;
    Type& operator=(const Type&) = delete;

    Kind kind() const { return this->m_kind; }
    TypeId id() const { return this->m_id; }
    const std::string& name() const { return this->m_name; }
    const Type* super() const { return this->m_super; }      // null only for Any, the root

    bool isUnknown() const { return this->m_kind == Kind::Unknown; }
    // Can a value of this type exist at runtime? Abstract types cannot be
    // instantiated; they exist to be dispatched on.
    bool isAbstract() const { return this->m_kind == Kind::Abstract; }
private:
    Kind m_kind;
    std::string m_name;
    const Type* m_super;
    TypeId m_id;
};

// ------------------------------------
// Simple Types
// ------------------------------------
class AbstractType: public Type {
public:
    AbstractType(std::string name, const Type* super): Type(Kind::Abstract, std::move(name), super) {}
};


/*
- Signedness: PrimitiveType doesn't store it. Today you'd have to infer it from the name, such as a leading U. I'd add a bool to PrimitiveType next to m_bit_width, set in the constructor, rather than keep string-matching names. fromContextType could then use it for INT vs UINT as well.
- Signed range: -2^(N-1) to 2^(N-1) - 1.
- Unsigned range: 0 to 2^N - 1.
- 64-bit case: 1 << 64 is undefined behavior in C++. For N = 64, signed needs no check because an int64 always fits. Unsigned only needs v >= 0.
- UInt64 limit: a literal above INT64_MAX can't be held in an int64 at all, so the lexer or parser would need to widen first. I'd leave that alone for now.

The check belongs in TypeResolver::visitVarDecl, after you visit the initializer and compare its type to the declared one. If the initializer is an IntLiteral and the declared type is a different integer type, check the range, then retype the literal node to the declared type with setNodeType. Otherwise it's a TypeMismatch. You'll need a new exception for the out-of-range case, like the ones in context/Exception.hpp.
*/

class PrimitiveType: public Type {
public:
    PrimitiveType(std::string name, const Type* super, std::size_t bitWidth, bool msigned): 
        Type(Kind::Primitive, std::move(name), super), m_bit_width(bitWidth), m_signed(msigned) {}
    
    bool isSigned() const { return this->m_signed; }

    // The width the declaration asks for, e.g. 64 for Int64. Distinct from
    // the ABI size and alignment the type gets on a given target, which the
    // DataLayout decides.
    std::size_t bitWidth() const { return this->m_bit_width; }
    std::int64_t minValue() const {
        if (this->isSigned()) {
            return static_cast<std::int64_t>(~std::uint64_t{0} << (this->m_bit_width - 1));
        }
        return 0;
    }

    // TODO: Handle booleans
    std::uint64_t maxValue() const {
        if (this->isSigned()) {
            // std::uint64_t{1} is undifined behavior
            return ~std::uint64_t{0} >> (65 - this->m_bit_width);
        }
        return ~std::uint64_t{0} >> (64 - this->m_bit_width);
    }
private:
    std::size_t m_bit_width;
    bool m_signed;
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
    StructType(std::string name, const Type* super, std::vector<StructType::Field> fields): Type(Kind::Struct, std::move(name), super), m_fields(std::move(fields)) {}
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
    TupleType(std::string name, const Type* super, std::vector<const Type*> elements): Type(Kind::Tuple, std::move(name), super), m_elements(std::move(elements)) {}
    const std::vector<const Type*>& elements() const { return this->m_elements; }
private:
    std::vector<const Type*> m_elements;
};


// Invariant: two function types are related only if identical.
class FunctionType: public Type {
public:
    FunctionType(std::string name, const Type* super, const TupleType* args, const Type* returns):
        Type(Kind::Function, std::move(name), super), m_args(args), m_returns(returns) {}

    const TupleType* args() const { return this->m_args; }
    const Type* returns() const { return this->m_returns; }

private:
    const TupleType* m_args;
    const Type* m_returns;
};


// a <: b  -- walk a's super chain looking for b
bool isSubtype(const Type* a, const Type* b);


} // namespace blast::core::context
