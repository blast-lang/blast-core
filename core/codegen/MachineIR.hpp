#pragma once
#include <cstdint>

#include <core/ir/IR.hpp>

// Target-level IR: physical registers, addressing modes, ABI-expanded
namespace blast::core::codegen {


class Register {
public:
    using RegId = std::uint32_t;
    enum class Class: std::uint8_t {
        INT,
        FLOAT,
    };
public:
    Register(RegId id, Class cls): m_id(id), m_class(cls) {}
    RegId id() const { return this->m_id; }
    Class cls() const { return this->m_class; }

private:
    RegId m_id;
    Class m_class;
};

// One width while aliasing is out of scope: every value occupies a full GPR.
enum class Width: std::uint8_t {
    // W8, W16, W32,
    W64,
};

enum class Cond: std::uint8_t {
    LT, LE, GT, GE, EQ, NE,
};

// Two-address code form
enum class Opcode: std::uint8_t {
    MOV,
    ADD, SUB, IMUL,
    CQO, IDIV,
    NEG,
    CMP, SETCC,
    JMP, JCC, CALL, RET,
    // Expanded once the frame size is known
    PROLOGUE, EPILOGUE,

    // Not covered for now:
    // LEA,
    // PUSH, POP,
    // MOVSX, MOVZX,
    // AND, OR, XOR, NOT,
    // SHL, SAR, SHR,
    // MOVSD, ADDSD, SUBSD, MULSD, DIVSD,
    // UCOMISD, CVTSI2SD, CVTTSD2SI,
};

} // namespace blast::core::codegen
