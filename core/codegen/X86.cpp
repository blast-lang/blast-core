#include <core/codegen/X86.hpp>
#include <core/Exception.hpp>

#include <vector>

namespace blast::core::codegen {

void X86::lower(const ir::Module& mod) {
    // TODO: Link fi last block to fi+1 first block ?
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

    MachineFunction& main = this->fcts()[0];
    MachineBlock& main_entry = main.blocks()[0];

    main_entry.addInstruction(MachineOpcode::PUSH, PREG(nullptr, {ir::Type::Kind::INT, ir::Type::Width::W64}), MNONE());

    // TODO: block labels and instructions
    for (const MachineFunction& fct: this->fcts()) {
        for (const MachineBlock& block: fct.blocks()) {
            
        }
    }

    this->m_out +=
        "    mov $5, %rsi\n"  // TODO: the value of 'a'
        "    lea .Lfmt(%rip), %rdi\n"
        "    xor %eax, %eax\n"
        "    call printf\n"
        "    xor %eax, %eax\n"
        "    pop %rbp\n"
        "    ret\n";
}


RegisterAllocator::RegisterAllocator() {
    // General purpose
    Register RAX(1, "RAX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register EAX(2, "EAX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register AX(3, "AX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register AH(4, "AH", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    Register AL(5, "AL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    RAX.addSubreg(EAX);
    EAX.addSubreg(AX);
    AX.addSubreg(AH);
    AX.addSubreg(AL);

    Register RCX(6, "RCX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register ECX(7, "ECX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register CX(8, "CX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register CH(9, "CH", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    Register CL(10, "CL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    RCX.addSubreg(ECX);
    ECX.addSubreg(CX);
    CX.addSubreg(CH);
    CX.addSubreg(CL);

    Register RDX(11, "RDX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register EDX(12, "EDX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register DX(13, "DX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register DH(14, "DH", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    Register DL(15, "DL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    RDX.addSubreg(EDX);
    EDX.addSubreg(DX);
    DX.addSubreg(DH);
    DX.addSubreg(DL);

    Register RBX(16, "RBX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register EBX(17, "EBX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register BX(18, "BX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register BH(19, "BH", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    Register BL(20, "BL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    RBX.addSubreg(EBX);
    EBX.addSubreg(BX);
    BX.addSubreg(BH);
    BX.addSubreg(BL);

    Register RSP(21, "RSP", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register ESP(22, "ESP", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register SP(23, "SP", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register SPL(24, "SPL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    RSP.addSubreg(ESP);
    ESP.addSubreg(SP);
    SP.addSubreg(SPL);

    Register RBP(25, "RBP", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register EBP(26, "EBP", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register BP(27, "BP", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register BPL(28, "BPL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    RBP.addSubreg(EBP);
    EBP.addSubreg(BP);
    BP.addSubreg(BPL);

    Register RSI(29, "RSI", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register ESI(30, "ESI", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register SI(31, "SI", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register SIL(32, "SIL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    RSI.addSubreg(ESI);
    ESI.addSubreg(SI);
    SI.addSubreg(SIL);

    Register RDI(33, "RDI", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register EDI(34, "EDI", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register DI(35, "DI", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register DIL(36, "DIL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    RDI.addSubreg(EDI);
    EDI.addSubreg(DI);
    DI.addSubreg(DIL);

    Register R8(37, "R8", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register R8D(38, "R8D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register R8W(39, "R8W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register R8B(40, "R8B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    R8.addSubreg(R8D);
    R8D.addSubreg(R8W);
    R8W.addSubreg(R8B);

    Register R9(41, "R9", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register R9D(42, "R9D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register R9W(43, "R9W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register R9B(44, "R9B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    R9.addSubreg(R9D);
    R9D.addSubreg(R9W);
    R9W.addSubreg(R9B);

    Register R10(45, "R10", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register R10D(46, "R10D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register R10W(47, "R10W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register R10B(48, "R10B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    R10.addSubreg(R10D);
    R10D.addSubreg(R10W);
    R10W.addSubreg(R10B);

    Register R11(49, "R11", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register R11D(50, "R11D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register R11W(51, "R11W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register R11B(52, "R11B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    R11.addSubreg(R11D);
    R11D.addSubreg(R11W);
    R11W.addSubreg(R11B);

    Register R12(53, "R12", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register R12D(54, "R12D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register R12W(55, "R12W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register R12B(56, "R12B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    R12.addSubreg(R12D);
    R12D.addSubreg(R12W);
    R12W.addSubreg(R12B);

    Register R13(57, "R13", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register R13D(58, "R13D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register R13W(59, "R13W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register R13B(60, "R13B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    R13.addSubreg(R13D);
    R13D.addSubreg(R13W);
    R13W.addSubreg(R13B);

    Register R14(61, "R14", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register R14D(62, "R14D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register R14W(63, "R14W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register R14B(64, "R14B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    R14.addSubreg(R14D);
    R14D.addSubreg(R14W);
    R14W.addSubreg(R14B);

    Register R15(65, "R15", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP);
    Register R15D(66, "R15D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP);
    Register R15W(67, "R15W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP);
    Register R15B(68, "R15B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP);
    R15.addSubreg(R15D);
    R15D.addSubreg(R15W);
    R15W.addSubreg(R15B);

    // Vector
    Register ZMM0(69, "ZMM0", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM0(70, "YMM0", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM0(71, "XMM0", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM0.addSubreg(YMM0);
    YMM0.addSubreg(XMM0);

    Register ZMM1(72, "ZMM1", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM1(73, "YMM1", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM1(74, "XMM1", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM1.addSubreg(YMM1);
    YMM1.addSubreg(XMM1);

    Register ZMM2(75, "ZMM2", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM2(76, "YMM2", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM2(77, "XMM2", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM2.addSubreg(YMM2);
    YMM2.addSubreg(XMM2);

    Register ZMM3(78, "ZMM3", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM3(79, "YMM3", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM3(80, "XMM3", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM3.addSubreg(YMM3);
    YMM3.addSubreg(XMM3);

    Register ZMM4(81, "ZMM4", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM4(82, "YMM4", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM4(83, "XMM4", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM4.addSubreg(YMM4);
    YMM4.addSubreg(XMM4);

    Register ZMM5(84, "ZMM5", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM5(85, "YMM5", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM5(86, "XMM5", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM5.addSubreg(YMM5);
    YMM5.addSubreg(XMM5);

    Register ZMM6(87, "ZMM6", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM6(88, "YMM6", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM6(89, "XMM6", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM6.addSubreg(YMM6);
    YMM6.addSubreg(XMM6);

    Register ZMM7(90, "ZMM7", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM7(91, "YMM7", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM7(92, "XMM7", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM7.addSubreg(YMM7);
    YMM7.addSubreg(XMM7);

    Register ZMM8(93, "ZMM8", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM8(94, "YMM8", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM8(95, "XMM8", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM8.addSubreg(YMM8);
    YMM8.addSubreg(XMM8);

    Register ZMM9(96, "ZMM9", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM9(97, "YMM9", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM9(98, "XMM9", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM9.addSubreg(YMM9);
    YMM9.addSubreg(XMM9);

    Register ZMM10(99, "ZMM10", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM10(100, "YMM10", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM10(101, "XMM10", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM10.addSubreg(YMM10);
    YMM10.addSubreg(XMM10);

    Register ZMM11(102, "ZMM11", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM11(103, "YMM11", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM11(104, "XMM11", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM11.addSubreg(YMM11);
    YMM11.addSubreg(XMM11);

    Register ZMM12(105, "ZMM12", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM12(106, "YMM12", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM12(107, "XMM12", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM12.addSubreg(YMM12);
    YMM12.addSubreg(XMM12);

    Register ZMM13(108, "ZMM13", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM13(109, "YMM13", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM13(110, "XMM13", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM13.addSubreg(YMM13);
    YMM13.addSubreg(XMM13);

    Register ZMM14(111, "ZMM14", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM14(112, "YMM14", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM14(113, "XMM14", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM14.addSubreg(YMM14);
    YMM14.addSubreg(XMM14);

    Register ZMM15(114, "ZMM15", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR);
    Register YMM15(115, "YMM15", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR);
    Register XMM15(116, "XMM15", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR);
    ZMM15.addSubreg(YMM15);
    YMM15.addSubreg(XMM15);
    // Mask
    Register K0(117, "K0", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK);
    Register K1(118, "K1", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK);
    Register K2(119, "K2", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK);
    Register K3(120, "K3", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK);
    Register K4(121, "K4", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK);
    Register K5(122, "K5", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK);
    Register K6(123, "K6", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK);
    Register K7(124, "K7", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK);


    this->m_registers.reserve(125);
    this->m_registers.push_back(Register(0, "NONE", ir::VOID(), RegClass::GP));
    this->m_registers.push_back(std::move(RAX));
    this->m_registers.push_back(std::move(EAX));
    this->m_registers.push_back(std::move(AX));
    this->m_registers.push_back(std::move(AH));
    this->m_registers.push_back(std::move(AL));
    this->m_registers.push_back(std::move(RCX));
    this->m_registers.push_back(std::move(ECX));
    this->m_registers.push_back(std::move(CX));
    this->m_registers.push_back(std::move(CH));
    this->m_registers.push_back(std::move(CL));
    this->m_registers.push_back(std::move(RDX));
    this->m_registers.push_back(std::move(EDX));
    this->m_registers.push_back(std::move(DX));
    this->m_registers.push_back(std::move(DH));
    this->m_registers.push_back(std::move(DL));
    this->m_registers.push_back(std::move(RBX));
    this->m_registers.push_back(std::move(EBX));
    this->m_registers.push_back(std::move(BX));
    this->m_registers.push_back(std::move(BH));
    this->m_registers.push_back(std::move(BL));
    this->m_registers.push_back(std::move(RSP));
    this->m_registers.push_back(std::move(ESP));
    this->m_registers.push_back(std::move(SP));
    this->m_registers.push_back(std::move(SPL));
    this->m_registers.push_back(std::move(RBP));
    this->m_registers.push_back(std::move(EBP));
    this->m_registers.push_back(std::move(BP));
    this->m_registers.push_back(std::move(BPL));
    this->m_registers.push_back(std::move(RSI));
    this->m_registers.push_back(std::move(ESI));
    this->m_registers.push_back(std::move(SI));
    this->m_registers.push_back(std::move(SIL));
    this->m_registers.push_back(std::move(RDI));
    this->m_registers.push_back(std::move(EDI));
    this->m_registers.push_back(std::move(DI));
    this->m_registers.push_back(std::move(DIL));
    this->m_registers.push_back(std::move(R8));
    this->m_registers.push_back(std::move(R8D));
    this->m_registers.push_back(std::move(R8W));
    this->m_registers.push_back(std::move(R8B));
    this->m_registers.push_back(std::move(R9));
    this->m_registers.push_back(std::move(R9D));
    this->m_registers.push_back(std::move(R9W));
    this->m_registers.push_back(std::move(R9B));
    this->m_registers.push_back(std::move(R10));
    this->m_registers.push_back(std::move(R10D));
    this->m_registers.push_back(std::move(R10W));
    this->m_registers.push_back(std::move(R10B));
    this->m_registers.push_back(std::move(R11));
    this->m_registers.push_back(std::move(R11D));
    this->m_registers.push_back(std::move(R11W));
    this->m_registers.push_back(std::move(R11B));
    this->m_registers.push_back(std::move(R12));
    this->m_registers.push_back(std::move(R12D));
    this->m_registers.push_back(std::move(R12W));
    this->m_registers.push_back(std::move(R12B));
    this->m_registers.push_back(std::move(R13));
    this->m_registers.push_back(std::move(R13D));
    this->m_registers.push_back(std::move(R13W));
    this->m_registers.push_back(std::move(R13B));
    this->m_registers.push_back(std::move(R14));
    this->m_registers.push_back(std::move(R14D));
    this->m_registers.push_back(std::move(R14W));
    this->m_registers.push_back(std::move(R14B));
    this->m_registers.push_back(std::move(R15));
    this->m_registers.push_back(std::move(R15D));
    this->m_registers.push_back(std::move(R15W));
    this->m_registers.push_back(std::move(R15B));
    this->m_registers.push_back(std::move(ZMM0));
    this->m_registers.push_back(std::move(YMM0));
    this->m_registers.push_back(std::move(XMM0));
    this->m_registers.push_back(std::move(ZMM1));
    this->m_registers.push_back(std::move(YMM1));
    this->m_registers.push_back(std::move(XMM1));
    this->m_registers.push_back(std::move(ZMM2));
    this->m_registers.push_back(std::move(YMM2));
    this->m_registers.push_back(std::move(XMM2));
    this->m_registers.push_back(std::move(ZMM3));
    this->m_registers.push_back(std::move(YMM3));
    this->m_registers.push_back(std::move(XMM3));
    this->m_registers.push_back(std::move(ZMM4));
    this->m_registers.push_back(std::move(YMM4));
    this->m_registers.push_back(std::move(XMM4));
    this->m_registers.push_back(std::move(ZMM5));
    this->m_registers.push_back(std::move(YMM5));
    this->m_registers.push_back(std::move(XMM5));
    this->m_registers.push_back(std::move(ZMM6));
    this->m_registers.push_back(std::move(YMM6));
    this->m_registers.push_back(std::move(XMM6));
    this->m_registers.push_back(std::move(ZMM7));
    this->m_registers.push_back(std::move(YMM7));
    this->m_registers.push_back(std::move(XMM7));
    this->m_registers.push_back(std::move(ZMM8));
    this->m_registers.push_back(std::move(YMM8));
    this->m_registers.push_back(std::move(XMM8));
    this->m_registers.push_back(std::move(ZMM9));
    this->m_registers.push_back(std::move(YMM9));
    this->m_registers.push_back(std::move(XMM9));
    this->m_registers.push_back(std::move(ZMM10));
    this->m_registers.push_back(std::move(YMM10));
    this->m_registers.push_back(std::move(XMM10));
    this->m_registers.push_back(std::move(ZMM11));
    this->m_registers.push_back(std::move(YMM11));
    this->m_registers.push_back(std::move(XMM11));
    this->m_registers.push_back(std::move(ZMM12));
    this->m_registers.push_back(std::move(YMM12));
    this->m_registers.push_back(std::move(XMM12));
    this->m_registers.push_back(std::move(ZMM13));
    this->m_registers.push_back(std::move(YMM13));
    this->m_registers.push_back(std::move(XMM13));
    this->m_registers.push_back(std::move(ZMM14));
    this->m_registers.push_back(std::move(YMM14));
    this->m_registers.push_back(std::move(XMM14));
    this->m_registers.push_back(std::move(ZMM15));
    this->m_registers.push_back(std::move(YMM15));
    this->m_registers.push_back(std::move(XMM15));
    this->m_registers.push_back(std::move(K0));
    this->m_registers.push_back(std::move(K1));
    this->m_registers.push_back(std::move(K2));
    this->m_registers.push_back(std::move(K3));
    this->m_registers.push_back(std::move(K4));
    this->m_registers.push_back(std::move(K5));
    this->m_registers.push_back(std::move(K6));
    this->m_registers.push_back(std::move(K7));
}

} // namespace blast::core::codegen
