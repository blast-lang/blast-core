#include <core/codegen/X86.hpp>
#include <core/Exception.hpp>

#include <algorithm>
#include <bit>
#include <iostream>
#include <iterator>
#include <set>
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

    for (const ir::Instruction& inst: block.instrs()) {
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
            // The returned value sits in m_src so liveness reads it as a use:
            // as a destination it would look like a write and kill the value.
            mblock.addInstruction(MachineOpcode::RET, MNONE(), lhs);
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

void allocate(MachineFunction& fct, const RegisterAllocator& regs);

RegisterAllocator::RegisterAllocator(X86& x86) {
    // General purpose
    Register RAX(1, "RAX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 0, false, true);
    Register EAX(2, "EAX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 0, false, true);
    Register AX(3, "AX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 0, false, true);
    Register AH(4, "AH", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 0, false, true);
    Register AL(5, "AL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 0, false, true);
    RAX.addSubreg(EAX);
    EAX.addSubreg(AX);
    AX.addSubreg(AH);
    AX.addSubreg(AL);

    Register RCX(6, "RCX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 1, false, true);
    Register ECX(7, "ECX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 1, false, true);
    Register CX(8, "CX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 1, false, true);
    Register CH(9, "CH", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 1, false, true);
    Register CL(10, "CL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 1, false, true);
    RCX.addSubreg(ECX);
    ECX.addSubreg(CX);
    CX.addSubreg(CH);
    CX.addSubreg(CL);

    Register RDX(11, "RDX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 2, false, true);
    Register EDX(12, "EDX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 2, false, true);
    Register DX(13, "DX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 2, false, true);
    Register DH(14, "DH", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 2, false, true);
    Register DL(15, "DL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 2, false, true);
    RDX.addSubreg(EDX);
    EDX.addSubreg(DX);
    DX.addSubreg(DH);
    DX.addSubreg(DL);

    Register RBX(16, "RBX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 3, false, false);
    Register EBX(17, "EBX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 3, false, false);
    Register BX(18, "BX", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 3, false, false);
    Register BH(19, "BH", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 3, false, false);
    Register BL(20, "BL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 3, false, false);
    RBX.addSubreg(EBX);
    EBX.addSubreg(BX);
    BX.addSubreg(BH);
    BX.addSubreg(BL);

    Register RSP(21, "RSP", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 4, true, false);
    Register ESP(22, "ESP", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 4, true, false);
    Register SP(23, "SP", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 4, true, false);
    Register SPL(24, "SPL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 4, true, false);
    RSP.addSubreg(ESP);
    ESP.addSubreg(SP);
    SP.addSubreg(SPL);

    Register RBP(25, "RBP", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 5, true, false);
    Register EBP(26, "EBP", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 5, true, false);
    Register BP(27, "BP", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 5, true, false);
    Register BPL(28, "BPL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 5, true, false);
    RBP.addSubreg(EBP);
    EBP.addSubreg(BP);
    BP.addSubreg(BPL);

    Register RSI(29, "RSI", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 6, false, true);
    Register ESI(30, "ESI", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 6, false, true);
    Register SI(31, "SI", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 6, false, true);
    Register SIL(32, "SIL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 6, false, true);
    RSI.addSubreg(ESI);
    ESI.addSubreg(SI);
    SI.addSubreg(SIL);

    Register RDI(33, "RDI", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 7, false, true);
    Register EDI(34, "EDI", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 7, false, true);
    Register DI(35, "DI", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 7, false, true);
    Register DIL(36, "DIL", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 7, false, true);
    RDI.addSubreg(EDI);
    EDI.addSubreg(DI);
    DI.addSubreg(DIL);

    Register R8(37, "R8", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 8, false, true);
    Register R8D(38, "R8D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 8, false, true);
    Register R8W(39, "R8W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 8, false, true);
    Register R8B(40, "R8B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 8, false, true);
    R8.addSubreg(R8D);
    R8D.addSubreg(R8W);
    R8W.addSubreg(R8B);

    Register R9(41, "R9", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 9, false, true);
    Register R9D(42, "R9D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 9, false, true);
    Register R9W(43, "R9W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 9, false, true);
    Register R9B(44, "R9B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 9, false, true);
    R9.addSubreg(R9D);
    R9D.addSubreg(R9W);
    R9W.addSubreg(R9B);

    Register R10(45, "R10", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 10, false, true);
    Register R10D(46, "R10D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 10, false, true);
    Register R10W(47, "R10W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 10, false, true);
    Register R10B(48, "R10B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 10, false, true);
    R10.addSubreg(R10D);
    R10D.addSubreg(R10W);
    R10W.addSubreg(R10B);

    Register R11(49, "R11", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 11, false, true);
    Register R11D(50, "R11D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 11, false, true);
    Register R11W(51, "R11W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 11, false, true);
    Register R11B(52, "R11B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 11, false, true);
    R11.addSubreg(R11D);
    R11D.addSubreg(R11W);
    R11W.addSubreg(R11B);

    Register R12(53, "R12", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 12, false, false);
    Register R12D(54, "R12D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 12, false, false);
    Register R12W(55, "R12W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 12, false, false);
    Register R12B(56, "R12B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 12, false, false);
    R12.addSubreg(R12D);
    R12D.addSubreg(R12W);
    R12W.addSubreg(R12B);

    Register R13(57, "R13", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 13, false, false);
    Register R13D(58, "R13D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 13, false, false);
    Register R13W(59, "R13W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 13, false, false);
    Register R13B(60, "R13B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 13, false, false);
    R13.addSubreg(R13D);
    R13D.addSubreg(R13W);
    R13W.addSubreg(R13B);

    Register R14(61, "R14", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 14, false, false);
    Register R14D(62, "R14D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 14, false, false);
    Register R14W(63, "R14W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 14, false, false);
    Register R14B(64, "R14B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 14, false, false);
    R14.addSubreg(R14D);
    R14D.addSubreg(R14W);
    R14W.addSubreg(R14B);

    Register R15(65, "R15", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W64}, RegClass::GP, 15, false, false);
    Register R15D(66, "R15D", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W32}, RegClass::GP, 15, false, false);
    Register R15W(67, "R15W", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W16}, RegClass::GP, 15, false, false);
    Register R15B(68, "R15B", {.m_kind = ir::Type::Kind::INT, .m_width = ir::Type::Width::W8}, RegClass::GP, 15, false, false);
    R15.addSubreg(R15D);
    R15D.addSubreg(R15W);
    R15W.addSubreg(R15B);

    // Vector
    Register ZMM0(69, "ZMM0", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 0, false, true);
    Register YMM0(70, "YMM0", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 0, false, true);
    Register XMM0(71, "XMM0", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 0, false, true);
    ZMM0.addSubreg(YMM0);
    YMM0.addSubreg(XMM0);

    Register ZMM1(72, "ZMM1", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 1, false, true);
    Register YMM1(73, "YMM1", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 1, false, true);
    Register XMM1(74, "XMM1", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 1, false, true);
    ZMM1.addSubreg(YMM1);
    YMM1.addSubreg(XMM1);

    Register ZMM2(75, "ZMM2", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 2, false, true);
    Register YMM2(76, "YMM2", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 2, false, true);
    Register XMM2(77, "XMM2", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 2, false, true);
    ZMM2.addSubreg(YMM2);
    YMM2.addSubreg(XMM2);

    Register ZMM3(78, "ZMM3", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 3, false, true);
    Register YMM3(79, "YMM3", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 3, false, true);
    Register XMM3(80, "XMM3", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 3, false, true);
    ZMM3.addSubreg(YMM3);
    YMM3.addSubreg(XMM3);

    Register ZMM4(81, "ZMM4", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 4, false, true);
    Register YMM4(82, "YMM4", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 4, false, true);
    Register XMM4(83, "XMM4", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 4, false, true);
    ZMM4.addSubreg(YMM4);
    YMM4.addSubreg(XMM4);

    Register ZMM5(84, "ZMM5", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 5, false, true);
    Register YMM5(85, "YMM5", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 5, false, true);
    Register XMM5(86, "XMM5", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 5, false, true);
    ZMM5.addSubreg(YMM5);
    YMM5.addSubreg(XMM5);

    Register ZMM6(87, "ZMM6", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 6, false, true);
    Register YMM6(88, "YMM6", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 6, false, true);
    Register XMM6(89, "XMM6", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 6, false, true);
    ZMM6.addSubreg(YMM6);
    YMM6.addSubreg(XMM6);

    Register ZMM7(90, "ZMM7", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 7, false, true);
    Register YMM7(91, "YMM7", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 7, false, true);
    Register XMM7(92, "XMM7", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 7, false, true);
    ZMM7.addSubreg(YMM7);
    YMM7.addSubreg(XMM7);

    Register ZMM8(93, "ZMM8", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 8, false, true);
    Register YMM8(94, "YMM8", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 8, false, true);
    Register XMM8(95, "XMM8", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 8, false, true);
    ZMM8.addSubreg(YMM8);
    YMM8.addSubreg(XMM8);

    Register ZMM9(96, "ZMM9", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 9, false, true);
    Register YMM9(97, "YMM9", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 9, false, true);
    Register XMM9(98, "XMM9", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 9, false, true);
    ZMM9.addSubreg(YMM9);
    YMM9.addSubreg(XMM9);

    Register ZMM10(99, "ZMM10", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 10, false, true);
    Register YMM10(100, "YMM10", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 10, false, true);
    Register XMM10(101, "XMM10", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 10, false, true);
    ZMM10.addSubreg(YMM10);
    YMM10.addSubreg(XMM10);

    Register ZMM11(102, "ZMM11", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 11, false, true);
    Register YMM11(103, "YMM11", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 11, false, true);
    Register XMM11(104, "XMM11", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 11, false, true);
    ZMM11.addSubreg(YMM11);
    YMM11.addSubreg(XMM11);

    Register ZMM12(105, "ZMM12", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 12, false, true);
    Register YMM12(106, "YMM12", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 12, false, true);
    Register XMM12(107, "XMM12", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 12, false, true);
    ZMM12.addSubreg(YMM12);
    YMM12.addSubreg(XMM12);

    Register ZMM13(108, "ZMM13", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 13, false, true);
    Register YMM13(109, "YMM13", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 13, false, true);
    Register XMM13(110, "XMM13", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 13, false, true);
    ZMM13.addSubreg(YMM13);
    YMM13.addSubreg(XMM13);

    Register ZMM14(111, "ZMM14", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 14, false, true);
    Register YMM14(112, "YMM14", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 14, false, true);
    Register XMM14(113, "XMM14", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 14, false, true);
    ZMM14.addSubreg(YMM14);
    YMM14.addSubreg(XMM14);

    Register ZMM15(114, "ZMM15", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W512}, RegClass::VECTOR, 15, false, true);
    Register YMM15(115, "YMM15", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W256}, RegClass::VECTOR, 15, false, true);
    Register XMM15(116, "XMM15", {.m_kind = ir::Type::Kind::FLOAT, .m_width = ir::Type::Width::W128}, RegClass::VECTOR, 15, false, true);
    ZMM15.addSubreg(YMM15);
    YMM15.addSubreg(XMM15);
    // Mask
    Register K0(117, "K0", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK, 0, false, true);
    Register K1(118, "K1", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK, 1, false, true);
    Register K2(119, "K2", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK, 2, false, true);
    Register K3(120, "K3", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK, 3, false, true);
    Register K4(121, "K4", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK, 4, false, true);
    Register K5(122, "K5", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK, 5, false, true);
    Register K6(123, "K6", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK, 6, false, true);
    Register K7(124, "K7", {.m_kind = ir::Type::Kind::UINT, .m_width = ir::Type::Width::W64}, RegClass::MASK, 7, false, true);


    this->m_registers.reserve(125);
    this->m_registers.push_back(Register(0, "NONE", ir::VOID(), RegClass::GP, 0, false, false));
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

    // Building successors
    for (MachineFunction& f: x86.fcts()){
        allocate(f, *this);
    }
}

const Register& RegisterAllocator::reg(std::string_view label) const {
    for (const Register& r: this->m_registers) {
        if (r.label() == label) {
            return r;
        }
    }
    throw CodegenError("[RegisterAllocator] Unknown register '" + std::string(label) + "'");
}


// https://cse.sc.edu/~mgv/csce531sp20/notes/mogensen_Ch8_Slides_register-allocation.pdf
void allocate(MachineFunction& fct, const RegisterAllocator& regs) {
    // The frame setup goes in before liveness runs, so the pass sees the same
    // instruction stream the emitter will print.
    const ir::Type w64 = {ir::Type::Kind::INT, ir::Type::Width::W64};
    const MachineOperand rbp = PREG(&regs.reg("RBP"), w64);
    const MachineOperand rsp = PREG(&regs.reg("RSP"), w64);
    std::vector<MachineInstruction>& entry = fct.blocks()[0].instrs();
    entry.insert(entry.begin(), {
        { MachineOpcode::PUSH, rbp, MNONE() },
        { MachineOpcode::MOV, rbp, rsp }
    });

    // The RET marker becomes the printf call the runtime still stands in for,
    // followed by the frame teardown. Built here because a fixed register only
    // exists once the register table does, and before liveness so the CALL is
    // part of the stream the allocator reasons about.
    const ir::Type w32 = {ir::Type::Kind::INT, ir::Type::Width::W32};
    const MachineOperand rsi = PREG(&regs.reg("RSI"), w64);
    const MachineOperand rdi = PREG(&regs.reg("RDI"), w64);
    const MachineOperand eax = PREG(&regs.reg("EAX"), w32);
    for (MachineBlock& block: fct.blocks()) {
        std::vector<MachineInstruction>& instrs = block.instrs();
        for (std::size_t k = 0; k < instrs.size(); ++k) {
            if (instrs[k].m_op != MachineOpcode::RET) {
                continue;
            }
            const MachineOperand value = instrs[k].m_src;
            std::vector<MachineInstruction> tail;
            if (value.m_kind != MachineOperand::Kind::NONE) {
                tail.push_back({ MachineOpcode::MOV, rsi, value });
            }
            tail.push_back({ MachineOpcode::LEA, rdi, RIP(".Lfmt", w64) });
            tail.push_back({ MachineOpcode::XOR, eax, eax });
            tail.push_back({ MachineOpcode::CALL, SYM("printf", ir::VOID()), MNONE() });
            tail.push_back({ MachineOpcode::XOR, eax, eax });
            tail.push_back({ MachineOpcode::POP, rbp, MNONE() });
            tail.push_back({ MachineOpcode::RET, MNONE(), MNONE() });
            const auto at = instrs.erase(instrs.begin() + static_cast<std::ptrdiff_t>(k));
            instrs.insert(at, tail.begin(), tail.end());
            k += tail.size() - 1;
        }
    }

    // Gives where each block starts as if
    // you concatenated all the blocks' instruction vectors into one list in layout order.
    std::vector<std::size_t> bases;
    bases.reserve(fct.blocks().size());
    std::size_t next = 0;
    for (const MachineBlock& block: fct.blocks()) {
        bases.push_back(next);
        next += block.instrs().size();
    }

    // First, for a given block, find its successor(s)
    std::unordered_map<ir::BlockId, std::vector<ir::BlockId>> block_succs;
    for (const MachineBlock& block: fct.blocks()) {
        for (ir::BlockId pred: block.preds()) {
            block_succs[pred].push_back(block.id());
        }
    }

    // Given an instruction `i`, give the set of instructions that comes after
    // succ(i) = {i + 1} for most instructions
    // succ(i) = {j} for labels and gotos
    // succ(i) = {j, k} for if/else/loops
    // succ(i) = {} is i is the end of the program
    std::unordered_map<MachineInstruction*, std::set<MachineInstruction*>> succ;
    for (MachineBlock& block: fct.blocks()) {
        if (block.instrs().empty()) {
            continue;
        }

        for (std::size_t i = 0; i + 1 < block.instrs().size(); i++) {
            succ[&block.instrs()[i]] = {&block.instrs()[i + 1]};
        }

        std::set<MachineInstruction*> targets;
        const auto it = block_succs.find(block.id());
        if (it != block_succs.end()) {
            for (ir::BlockId s: it->second) {
                MachineBlock& target = fct.getBlock(s);
                // The successor instruction of a JUMP is the next block's first intruction
                if (!target.instrs().empty()) {
                    targets.insert(&target.instrs().front());
                }
            }
        }
        succ[&block.instrs().back()] = std::move(targets);
    }

    // Now, we will compute liveliness of operands
    // We define use[i] and wrt[i]:
    // use[i] is the set of operand (register) used/read by instruction i
    // wrt[i] is the set of operand (register) overwritten by instruction i
    auto isVreg = [](const MachineOperand& operand) {
        return operand.m_kind == MachineOperand::Kind::VREG;
    };

    auto isPreg = [](const MachineOperand& operand) {
        return operand.m_kind == MachineOperand::Kind::PREG;
    };

    // Liveness is about values, not operand slots: the same vreg read by two
    // instructions must compare equal
    auto regId = [](const MachineOperand& operand) -> ir::ValueId {
        return operand.m_vreg;
    };

    // A physical register is never a value to colour, it is a constraint, so it
    // is kept out of use/wrt and collected as what the instruction destroys.
    std::unordered_map<MachineInstruction*, std::set<ir::ValueId>> use;
    std::unordered_map<MachineInstruction*, std::set<ir::ValueId>> wrt;
    // clobber[i] is the set of physical register ids that instruction i destroys
    std::unordered_map<MachineInstruction*, std::set<ir::ValueId>> clobber;
    for (auto& [instr, s]: succ) {
        use[instr] = {};
        wrt[instr] = {};
        clobber[instr] = {};
        if (
            instr->m_op == MachineOpcode::MOV ||
            instr->m_op == MachineOpcode::ADD ||
            instr->m_op == MachineOpcode::IMUL ||
            instr->m_op == MachineOpcode::XOR
        ) {
            if (isVreg(instr->m_src)) {
                use[instr].insert(regId(instr->m_src));
            }
            if (isVreg(instr->m_dst)) {
                wrt[instr].insert(regId(instr->m_dst));
            }
        }
        if (isPreg(instr->m_dst)) {
            clobber[instr].insert(instr->m_dst.m_preg->id());
        }
        // A callee may return having trashed every caller-saved register
        if (instr->m_op == MachineOpcode::CALL) {
            for (const Register& r: regs.registers()) {
                if (r.callerSaved()) {
                    clobber[instr].insert(r.id());
                }
            }
        }
    }

    // We now define in[i] and out[i]:
    // in[i] is the set of operand that are live at the start of instruction i
    // out[i] is the set of operand that are live at the end of instruction i
    std::unordered_map<MachineInstruction*, std::set<ir::ValueId>> in;
    std::unordered_map<MachineInstruction*, std::set<ir::ValueId>> out;
    // Defined as:
    // in[i] = use[i] || (out[i] \ wrt[i])
    // out[i] = Union(in[j]) for j in succ[i]
    // Those two are mutually recursive, so they are solved by iterating from the
    // empty sets until a whole sweep changes nothing
    std::vector<MachineInstruction*> order;
    for (MachineBlock& block: fct.blocks()) {
        for (MachineInstruction& instr: block.instrs()) {
            order.push_back(&instr);
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        // Liveness flows backward: we do a reverse sweep
        for (auto it = order.rbegin(); it != order.rend(); ++it) {
            MachineInstruction* i = *it;
            // Union(in[j]) for j in succ[i]
            std::set<ir::ValueId> next_out;
            for (MachineInstruction* j: succ[i]) {
                next_out.insert(in[j].begin(), in[j].end());
            }

            std::set<ir::ValueId> next_in;
            // out[i] \ wrt[i]
            std::set_difference(
                next_out.begin(), next_out.end(),
                wrt[i].begin(), wrt[i].end(),
                std::inserter(next_in, next_in.end())
            );
            // use[i] || <above>
            next_in.insert(use[i].begin(), use[i].end());

            if (next_in != in[i] || next_out != out[i]) {
                in[i] = std::move(next_in);
                out[i] = std::move(next_out);
                changed = true;
            }
        }
    }

    // List all vregs in the function
    std::set<ir::ValueId> operands;
    // Get VREG information
    std::unordered_map<ir::ValueId, ir::Type> types;
    for (auto it = order.begin(); it != order.end(); ++it) {
        MachineInstruction* i = *it;
        if (i->m_src.m_kind == MachineOperand::Kind::VREG) {
            types[regId(i->m_src)] = i->m_src.m_type;
            operands.insert(regId(i->m_src));
        }
        if (i->m_dst.m_kind == MachineOperand::Kind::VREG) {
            types[regId(i->m_dst)] = i->m_dst.m_type;
            operands.insert(regId(i->m_dst));
        }
    }

    // Now we build the interference graph
    // Operand (register) x intefer with y if there is an intruction i that verify
    // x in wrt[i] AND y in out[i] AND x != y
    std::unordered_map<ir::ValueId, std::set<ir::ValueId>> inter_graph;
    for (ir::ValueId x: operands) {
        inter_graph[x] = {};
    }
    for (auto it = order.begin(); it != order.end(); ++it) {
        MachineInstruction* i = *it;
        for (ir::ValueId x: operands) {
            for (ir::ValueId y: operands) {
                if (x != y && wrt[i].find(x) != wrt[i].end() && out[i].find(y) != out[i].end()) {
                    inter_graph[x].insert(y);
                    inter_graph[y].insert(x);
                }
            }
        }
    }

    // Coloring
    // Domains is the list of colors (PREG) an operand (VREG) can take
    std::unordered_map<ir::ValueId, std::set<ir::ValueId>> domains;
    for (ir::ValueId x: operands) {
        domains[x] = {};
    }

    // For an operand x, we will add to its domain ONLY registers that:
    // 1) are not reserved
    // 2) Are the same type
    // 3) Which with is >= of the operand's witdh
    for (ir::ValueId x: operands) {
        for(const Register& r: regs.registers()) {
            if(
                !r.reserved() &&
                r.type().m_kind == types[x].m_kind &&
                r.type().m_width >= types[x].m_width
            ) {
                domains.at(x).insert(r.id());
            }
        }
    }

    // A value live across an instruction cannot sit in a register that
    // instruction destroys
    for (MachineInstruction* i: order) {
        for (ir::ValueId y: out[i]) {
            for (ir::ValueId r: clobber[i]) {
                domains.at(y).erase(r);
            }
        }
    }

    // Now, the algorithm
    const auto K = 16;
    // Find an operand which has < K neighboors
    // Those node can be colored without spilling
    std::unordered_map<ir::ValueId, ir::ValueId> colors;
    do {
        // sort operands by their remaining domain size
        // That way we color node with smallest domain first
        std::vector<ir::ValueId> by_domain;
        for (const auto& [node, neighboors]: inter_graph) {
            by_domain.push_back(node);
        }
        std::sort(by_domain.begin(), by_domain.end(), [&domains](ir::ValueId a, ir::ValueId b) {
            return domains.at(a).size() < domains.at(b).size();
        });

        auto it = std::find_if(
            by_domain.begin(), by_domain.end(),
            [&](ir::ValueId node) {
                return inter_graph.at(node).size() < K;
            }
        );

        // No more node to treat
        if (it == by_domain.end()) {
            break;
        }

        // No colors for x, need spilling
        if (domains.at(*it).empty()) {

        } else {
            // Color the node with it first available color
            colors[*it] = *domains.at(*it).begin();
            // This color can then be removed from the node neighboor's domain
            for(ir::ValueId n: inter_graph.at(*it)) {
                domains.at(n).erase(*domains.at(*it).begin());
            }
            // This node that then be removed from the interferance graph
            for (ir::ValueId n: inter_graph.at(*it)) {
                inter_graph.at(n).erase(*it);
            }
            inter_graph.erase(*it);
        }
        // TODO:
        // Register aliasing is unused. family() and subregs() are never read by allocate(), so colors are per register id: nothing stops %0 getting RAX and %1 getting EAX, which are the same 64 bits. Everything is i64 today so you can't hit it yet, but it's wrong the moment a narrower type appears. Two values interfere means their families must differ, not their ids.



    } while (!inter_graph.empty());

    // Apply the coloring by changing VREG into PREG!
    for (auto it = order.begin(); it != order.end(); ++it) {
        MachineInstruction* i = *it;
        if (i->m_src.m_kind == MachineOperand::Kind::VREG) {
            i->m_src = PREG(&regs.registers()[colors.at(regId(i->m_src))], i->m_src.m_type);
        }
        if (i->m_dst.m_kind == MachineOperand::Kind::VREG) {
            i->m_dst = PREG(&regs.registers()[colors.at(regId(i->m_dst))], i->m_dst.m_type);
        }
    }
}



} // namespace blast::core::codegen
