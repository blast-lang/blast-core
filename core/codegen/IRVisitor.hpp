#pragma once
#include <type_traits>

#include <core/ir/IR.hpp>

namespace blast::core::codegen {

// Structural walk over the IR: module -> function -> block -> instruction.
// Every level's default recurses into its children, so a Derived overrides
// only the level it cares about; not calling the base stops the descent.
template <typename Derived, typename R = void>
class IRVisitor {
public:
    R visitEmpty() {
        if constexpr (!std::is_void_v<R>) {
            return R{};
        }
    }

    R visitModule(const ir::Module& mod) {
        for (const ir::Function& fn: mod.fcts()) {
            self().visitFunction(fn);
        }
        return self().visitEmpty();
    }

    R visitFunction(const ir::Function& fn) {
        for (const ir::BasicBlock& block: fn.blocks()) {
            self().visitBlock(block);
        }
        return self().visitEmpty();
    }

    R visitBlock(const ir::BasicBlock& block) {
        for (const ir::Instruction& instr: block.instrs()) {
            self().visitInstruction(instr);
        }
        return self().visitEmpty();
    }

    // Per-opcode hooks, generated from the same list as the enum so a new
    // opcode cannot be added without one. Each falls back to visitOpcode.
#define BLAST_IR_VISITOR_HOOK(name)                                     \
    R visit##name(const ir::Instruction& instr) {                       \
        return self().visitOpcode(instr);                               \
    }
    BLAST_IR_OPCODES(BLAST_IR_VISITOR_HOOK)
#undef BLAST_IR_VISITOR_HOOK

    R visitOpcode(const ir::Instruction&) { return self().visitEmpty(); }

    // Override this instead of the hooks to handle every instruction alike.
    R visitInstruction(const ir::Instruction& instr) {
        switch (instr.op()) {
#define BLAST_IR_VISITOR_CASE(name)                                     \
        case ir::Opcode::name: return self().visit##name(instr);
            BLAST_IR_OPCODES(BLAST_IR_VISITOR_CASE)
#undef BLAST_IR_VISITOR_CASE
        }
        // Only reachable if an Opcode was added outside BLAST_IR_OPCODES.
        return self().visitEmpty();
    }

private:
    Derived& self() { return static_cast<Derived&>(*this); }
};

} // namespace blast::core::codegen
