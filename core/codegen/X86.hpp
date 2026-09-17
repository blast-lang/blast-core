#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <core/Exception.hpp>
#include <core/ir/IR.hpp>

namespace blast::core::codegen {

enum class Reg: std::uint8_t {
    NONE,
    RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
    R8, R9, R10, R11, R12, R13, R14, R15
};

enum class RegClass: std::uint8_t {
    GP,
    FP,
    VECTOR,
    MASK
};

// Phyical register, as a type (int/float) and a size
// Also store information about aliasing
class Register {
public:
    Register(ir::ValueId id, std::string label, ir::Type type, RegClass cls):
        m_id(id),
        m_label(std::move(label)),
        m_type(type),
        m_class(cls),
        m_parent(0),
        m_subreg()
    {}

    ir::ValueId id() const { return this->m_id; }
    const std::string& label() const { return this->m_label; }
    ir::Type type() const { return this->m_type; }
    RegClass regClass() const { return this->m_class; }

    ir::ValueId parent() const { return this->m_parent; }
    void setParent(ir::ValueId parent) { this->m_parent = parent; }

    const std::vector<ir::ValueId>& subregs() const { return this->m_subreg; }
    std::vector<ir::ValueId>& subregs() { return this->m_subreg; }

    void addSubreg(Register& sub) {
        sub.setParent(this->m_id);
        this->m_subreg.push_back(sub.id());
    }

private:
    ir::ValueId m_id;
    std::string m_label;
    ir::Type m_type;
    RegClass m_class;
    // Tree-like structure to store aliasing information
    ir::ValueId m_parent;
    std::vector<ir::ValueId> m_subreg;
};

struct MachineOperand {
    enum class Kind: std::uint8_t {
        NONE,
        // Virtual register
        VREG,
        // Physical register after regestir allocation
        PREG,
        LIT
    };

    Kind     m_kind;
    ir::Type m_type;
    union {
        ir::ValueId     m_vreg;
        const Register* m_preg;
        ir::Lireral     m_lit;
    };
};

inline MachineOperand MNONE() {
    return {
        .m_kind = MachineOperand::Kind::NONE,
        .m_type = ir::VOID(),
        .m_lit = { .m_i64 = 0 }
    };
}

inline MachineOperand VREG(ir::ValueId v, ir::Type t) {
    return {
        .m_kind = MachineOperand::Kind::VREG,
        .m_type = t,
        .m_vreg = v
    };
}

inline MachineOperand PREG(const Register* r, ir::Type t) {
    return {
        .m_kind = MachineOperand::Kind::PREG,
        .m_type = t,
        .m_preg = r
    };
}

inline MachineOperand LIT(ir::Lireral l, ir::Type t) {
    return {
        .m_kind = MachineOperand::Kind::LIT,
        .m_type = t,
        .m_lit = l
    };
}

// x86 is two-address: dst is also the left operand
enum class MachineOpcode: std::uint8_t {
    MOV,
    ADD,
    IMUL,
    XOR,
    CALL,
    PUSH,
    POP,
    RET,
    LEA,
};

struct MachineInstruction {
    MachineOpcode  m_op;
    MachineOperand m_dst;
    MachineOperand m_src;
};

// Block ids and labels mirror the ir::BasicBlock they were lowered from, so the
// CFG carries over unchanged.
class MachineBlock {
private:
    ir::BlockId m_id;
    std::string m_label;
    std::vector<MachineInstruction> m_instrs;
    std::vector<ir::BlockId> m_preds;

public:
    MachineBlock(ir::BlockId id, std::string label): m_id(id), m_label(std::move(label)), m_instrs(), m_preds() {}

    ir::BlockId id() const { return this->m_id; }
    const std::string& label() const { return this->m_label; }

    std::vector<MachineInstruction>& instrs() { return this->m_instrs; }
    const std::vector<MachineInstruction>& instrs() const { return this->m_instrs; }

    std::vector<ir::BlockId>& preds() { return this->m_preds; }
    const std::vector<ir::BlockId>& preds() const { return this->m_preds; }

    void addInstruction(MachineOpcode op, MachineOperand dst, MachineOperand src) {
        this->m_instrs.push_back({ op, dst, src });
    }
};

class MachineFunction {
private:
    ir::FctId m_id;
    std::string m_name;
    std::vector<MachineBlock> m_blocks;
    // Blocks are stored in lowering order, so an IR block id is not an index
    std::unordered_map<ir::BlockId, std::size_t> m_block_index;
    ir::ValueId m_next_vreg;

public:
    // next_vreg carries over the IR function's value counter: vregs are IR value
    // ids until an allocator says otherwise, so temps must not reuse one.
    MachineFunction(ir::FctId id, std::string name, ir::ValueId next_vreg): m_id(id), m_name(std::move(name)), m_blocks(), m_block_index(), m_next_vreg(next_vreg) {}

    ir::FctId id() const { return this->m_id; }
    const std::string& name() const { return this->m_name; }

    std::vector<MachineBlock>& blocks() { return this->m_blocks; }
    const std::vector<MachineBlock>& blocks() const { return this->m_blocks; }

    MachineBlock& addBlock(ir::BlockId id, std::string label) {
        this->m_block_index[id] = this->m_blocks.size();
        this->m_blocks.push_back(MachineBlock(id, std::move(label)));
        return this->m_blocks.back();
    }

    const MachineBlock& getBlock(ir::BlockId bid) const {
        const auto it = this->m_block_index.find(bid);
        if (it == this->m_block_index.end()) {
            throw CodegenError("[MachineFunction] Unknown block id " + std::to_string(bid) + " in '" + this->m_name + "'");
        }
        return this->m_blocks[it->second];
    }

    MachineBlock& getBlock(ir::BlockId bid) {
        const MachineFunction& self = *this;
        return const_cast<MachineBlock&>(self.getBlock(bid));
    }

    ir::ValueId newVreg() { return this->m_next_vreg++; }
};

class X86 {
private:
    std::vector<MachineFunction> m_fcts;
    std::string m_out;

public:
    X86() = default;

    std::vector<MachineFunction>& fcts() { return this->m_fcts; }
    const std::vector<MachineFunction>& fcts() const { return this->m_fcts; }
    const std::string& out() const { return this->m_out; }

    void lower(const ir::Module& mod);
    void emit();

private:
    void lowerFct(const ir::Function& fct);
    void lowerBlock(MachineFunction& mfct, const ir::BasicBlock& block, const std::vector<bool>& reachable);
    void lowerInstruction(MachineFunction& mfct, MachineBlock& mblock, const ir::Instruction& inst);

    MachineOperand lowerOperand(MachineFunction& mfct, ir::Operand op);
};

// m_registers is indexed by Register::id(), so slot 0 is a placeholder standing
// for 'no register'. It is never resized after construction: MachineOperand
// holds pointers into it.
class RegisterAllocator {
private:
    std::vector<Register> m_registers;

public:
    RegisterAllocator(X86& x86);

    const std::vector<Register>& registers() const { return this->m_registers; }
    const Register& reg(ir::ValueId id) const { return this->m_registers[id]; }
};


} // namespace blast::core::codegen
