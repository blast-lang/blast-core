#pragma once
#include <core/codegen/MachineIR.hpp>
#include <core/ir/IR.hpp>

// x86-64 target: the opcodes, and the lowering from IR into two-address form.
namespace blast::core::codegen::x86_64 {

enum class Opcode: TargetOpcode {
    MOV,
    ADD, SUB, IMUL,
    CQO, DIV, IDIV,
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

enum class Cond: std::uint8_t {
    LT, LE, GT, GE, EQ, NE,
};

constexpr TargetOpcode opcode(Opcode o) {
    return static_cast<TargetOpcode>(o);
}

const char* mnemonic(TargetOpcode op);

Width width(ir::Type t);
bool isSigned(ir::Type t);
RegClass regClass(ir::Type t);

MachineIR lower(const ir::Module& m);

} // namespace blast::core::codegen::x86_64
