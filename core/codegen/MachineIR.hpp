#pragma once
#include <cstdint>
#include <vector>
#include <string>

#include <core/ir/IR.hpp>

// Target-level IR: physical registers, addressing modes, ABI-expanded
namespace blast::core::codegen {


using RegisterId = std::uint32_t;
using MachineBlockId = std::uint32_t;

class Register {
public:
    enum class Class: std::uint8_t {
        INT,
        FLOAT,
    };
public:
    Register(RegisterId id, Class cls): m_id(id), m_class(cls) {}
    RegisterId id() const { return this->m_id; }
    Class cls() const { return this->m_class; }

private:
    RegisterId m_id;
    Class m_class;
};

// One width while aliasing is out of scope: every value occupies a full GPR.
enum class Width: std::uint8_t {
    W8, W16, W32,
    W64,
};

enum class Cond: std::uint8_t {
    LT, LE, GT, GE, EQ, NE,
};

// Two-address code form
enum class MachineOpcode: std::uint8_t {
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

class MachineOperand {
public:
    enum class Kind: std::uint8_t {
        NONE,
        // Virtual (not-yet-allocated) register
        VREG,
        // Proper allocated register
        REG,
        LABEL,
        LITERAL
    };

public:
    MachineOperand(const ir::Operand& op);

public:
    Kind kind() const { return this->m_kind; }
    RegisterId registerId() const { return this->m_value; }
    MachineBlockId blockId() const { return this->m_block; }
    const ir::Lireral& literal() const { return this->m_lit; }

private:
    Kind m_kind;
    union {
        RegisterId  m_value;   // REGISTER
        MachineBlockId  m_block;   // BLOCK
        ir::Lireral  m_lit;     // LITERAL
    };
};


class MachineInstr {
public:
    MachineInstr(const ir::Instruction& intr);

public:
    MachineOpcode op() const { return this->m_op; }
    Width width() const { return this->m_width; }
    const MachineOperand& lhs() const { return this->m_lhs; }
    const MachineOperand& rhs() const { return this->m_rhs; }

private:
    MachineOpcode m_op;
    Width m_width;
    MachineOperand m_lhs;
    MachineOperand m_rhs;
};

class MachineBlock {
public:
    MachineBlock(const ir::BasicBlock& block);

public:
    MachineBlockId id() const { return this->m_id; }
    const std::string& label() const { return this->m_label; }
    const std::vector<MachineInstr>& instrs() const { return this->m_instrs; }
    const std::vector<ir::BlockId>& preds() const { return this->m_preds; }
    const std::vector<ir::BlockId>& succs() const { return this->m_succs; }

private:
    MachineBlockId m_id;
    std::string m_label;
    std::vector<MachineInstr> m_instrs;
    std::vector<ir::BlockId> m_preds;
    std::vector<ir::BlockId> m_succs;
};

class MachineIR {
public:
    MachineIR(const ir::Module& m);

public:
    const std::vector<MachineBlock>& blocks() const { return this->m_blocks; }

private:
    std::vector<MachineBlock> m_blocks;
};

} // namespace blast::core::codegen
