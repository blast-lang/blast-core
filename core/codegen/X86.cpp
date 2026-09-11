#include <core/codegen/X86.hpp>
#include <core/Exception.hpp>

#include <vector>

namespace blast::core::codegen {

void X86::lower(const ir::Module& mod) {
    for (const ir::Function& fct : mod.fcts()) {
        this->lowerFct(fct);
    }
}

void X86::lowerFct(const ir::Function& fct) {
    this->m_fcts.push_back(MachineFunction(fct.id(), fct.name(), fct.nextValue()));
    MachineFunction& mfct = this->m_fcts.back();

    // Function ALWAYS have an entry block
    const ir::BasicBlock& entry = fct.getBlock(0);

    // Find the successors of the entry block to perform a depth-first search
    // And lower the blocks in postorder
    std::vector<ir::BlockId> order;
    order.reserve(fct.blocks().size());

    std::vector<bool> visited(fct.blocks().size(), false);
    auto visitBlock = [&](auto&& self, const ir::BasicBlock& b) -> void {
        visited[b.id()] = true;
        for (ir::BlockId succ: b.successors()) {
            if (!visited[succ]) {
                self(self, fct.getBlock(succ));
            }
        }
        order.push_back(b.id());
    };
    visitBlock(visitBlock, entry);
    // Now, lower in postorder
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        this->lowerBlock(mfct, fct.getBlock(*it), visited);
    }
}

void X86::lowerBlock(MachineFunction& mfct, const ir::BasicBlock& block, const std::vector<bool>& reachable) {
    MachineBlock& mblock = mfct.addBlock(block.id(), mfct.name() + "__" + block.label());

    for (ir::BlockId pred: block.preds()) {
        if (reachable[pred]) {
            mblock.preds().push_back(pred);
        }
    }

    for (const ir::Instruction& inst : block.instrs()) {
        this->lowerInstruction(mfct, mblock, inst);
    }
}

void X86::lowerInstruction(MachineFunction& mfct, MachineBlock& mblock, const ir::Instruction& inst) {
    const MachineOperand dst = this->lowerOperand(mfct, inst.result());
    const MachineOperand lhs = this->lowerOperand(mfct, inst.lhs());
    const MachineOperand rhs = this->lowerOperand(mfct, inst.rhs());

    switch (inst.op()) {
        case ir::Opcode::ADD:
            mblock.addInstruction(MachineOpcode::MOV, dst, lhs);
            mblock.addInstruction(MachineOpcode::ADD, dst, rhs);
            break;
        case ir::Opcode::MUL:
            mblock.addInstruction(MachineOpcode::MOV, dst, lhs);
            mblock.addInstruction(MachineOpcode::IMUL, dst, rhs);
            break;
        case ir::Opcode::RET:
            // TODO!
            break;
        default:
            throw CodegenError("[X86] Unsupported opcode");
    }
}

MachineOperand X86::lowerOperand(MachineFunction& mfct, ir::Operand op) {
    switch (op.m_kind) {
        case ir::Operand::Kind::NONE:
            return MNONE();
        case ir::Operand::Kind::REGISTER:
            return VREG(op.m_value, op.m_type);
        case ir::Operand::Kind::LITERAL:
            return LIT(op.m_lit, op.m_type);
        default:
            throw CodegenError("[X86] Unsupported operand kind");
    }
}

void X86::emit() {
    this->m_out =
        "    .section .note.GNU-stack,\"\",@progbits\n"
        "\n"
        "    .section .rodata\n"
        ".Lfmt:\n"
        "    .string \"a = %ld\\n\"\n"
        "\n"
        "    .text\n"
        "    .globl main\n"
        "main:\n"
        "    push %rbp\n"
        "    mov %rsp, %rbp\n";

    // TODO: block labels and instructions

    this->m_out +=
        "    mov $0, %rsi\n"  // TODO: the value of 'a'
        "    lea .Lfmt(%rip), %rdi\n"
        "    xor %eax, %eax\n"
        "    call printf\n"
        "    xor %eax, %eax\n"
        "    pop %rbp\n"
        "    ret\n";
}

} // namespace blast::core::codegen
