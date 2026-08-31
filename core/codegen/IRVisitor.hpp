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

    // Per-opcode hooks, each falling back to visitOpcode.
    R visitADD(const ir::Instruction& instr)  { return self().visitOpcode(instr); }
    R visitSUB(const ir::Instruction& instr)  { return self().visitOpcode(instr); }
    R visitMUL(const ir::Instruction& instr)  { return self().visitOpcode(instr); }
    R visitDIV(const ir::Instruction& instr)  { return self().visitOpcode(instr); }
    R visitNEG(const ir::Instruction& instr)  { return self().visitOpcode(instr); }
    R visitLT(const ir::Instruction& instr)   { return self().visitOpcode(instr); }
    R visitLE(const ir::Instruction& instr)   { return self().visitOpcode(instr); }
    R visitGT(const ir::Instruction& instr)   { return self().visitOpcode(instr); }
    R visitGE(const ir::Instruction& instr)   { return self().visitOpcode(instr); }
    R visitEQ(const ir::Instruction& instr)   { return self().visitOpcode(instr); }
    R visitNE(const ir::Instruction& instr)   { return self().visitOpcode(instr); }
    R visitCOPY(const ir::Instruction& instr) { return self().visitOpcode(instr); }
    R visitCALL(const ir::Instruction& instr) { return self().visitOpcode(instr); }
    R visitBR(const ir::Instruction& instr)   { return self().visitOpcode(instr); }
    R visitCBR(const ir::Instruction& instr)  { return self().visitOpcode(instr); }
    R visitRET(const ir::Instruction& instr)  { return self().visitOpcode(instr); }

    R visitOpcode(const ir::Instruction&) { return self().visitEmpty(); }

    // Override this instead of the hooks to handle every instruction alike.
    R visitInstruction(const ir::Instruction& instr) {
        switch (instr.op()) {
            case ir::Opcode::ADD:  return self().visitADD(instr);
            case ir::Opcode::SUB:  return self().visitSUB(instr);
            case ir::Opcode::MUL:  return self().visitMUL(instr);
            case ir::Opcode::DIV:  return self().visitDIV(instr);
            case ir::Opcode::NEG:  return self().visitNEG(instr);
            case ir::Opcode::LT:   return self().visitLT(instr);
            case ir::Opcode::LE:   return self().visitLE(instr);
            case ir::Opcode::GT:   return self().visitGT(instr);
            case ir::Opcode::GE:   return self().visitGE(instr);
            case ir::Opcode::EQ:   return self().visitEQ(instr);
            case ir::Opcode::NE:   return self().visitNE(instr);
            case ir::Opcode::COPY: return self().visitCOPY(instr);
            case ir::Opcode::CALL: return self().visitCALL(instr);
            case ir::Opcode::BR:   return self().visitBR(instr);
            case ir::Opcode::CBR:  return self().visitCBR(instr);
            case ir::Opcode::RET:  return self().visitRET(instr);
        }
        return self().visitEmpty();
    }

private:
    Derived& self() { return static_cast<Derived&>(*this); }
};

} // namespace blast::core::codegen


/*
Under SSA, each value has exactly one definition, so its live range is a connected subtree of the dominator tree.
Two such ranges interfere only if one's definition dominates the other's — which makes the interference graph chordal, and chordal graphs are colorable greedily in polynomial time with exactly ω colors, ω being the max clique = max register pressure at any program point. 
So there's no backtracking and no iterate-and-rebuild loop: once you've spilled enough to get pressure ≤ k everywhere, coloring cannot fail.
*/
