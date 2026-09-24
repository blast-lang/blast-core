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
    // Every block exists before any instruction is lowered: a jump in the entry
    // block names a label that only gets lowered later.
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        const ir::BasicBlock& b = fct.getBlock(*it);
        mfct.addBlock(b.id(), mfct.name() + "__" + b.label());
    }
    // Now, lower in postorder
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        this->lowerBlock(mfct, fct, fct.getBlock(*it), visited);
    }

    // The new entry block it still at the begining
    std::vector<MachineInstruction>& new_entry = mfct.blocks().front().instrs();
    // Inject the 'save' frame (stack) instructions
    const MachineOperand rbp = PREG(&this->getReg("RBP"), {ir::Type::Kind::INT, ir::Type::Width::W64});
    const MachineOperand rsp = PREG(&this->getReg("RSP"), {ir::Type::Kind::INT, ir::Type::Width::W64});
    const MachineOperand eax = PREG(&this->getReg("EAX"), {ir::Type::Kind::INT, ir::Type::Width::W32});
    new_entry.insert(new_entry.begin(), {
        { MachineOpcode::PUSH, MNONE(), rbp },
        { MachineOpcode::MOV, rbp, rsp }
    });
    // Same logic for the final block (frame teardown)
    std::vector<MachineInstruction>& new_exit = mfct.blocks().back().instrs();
    // Insert stack pointet restore BEFORE the return statement
    new_exit.insert(new_exit.end()-1, {
        { MachineOpcode::XOR, eax, eax },
        { MachineOpcode::POP, rbp, MNONE() },
    });
}

void X86::lowerBlock(MachineFunction& mfct, const ir::Function& fct, const ir::BasicBlock& block, const std::vector<bool>& reachable) {
    MachineBlock& mblock = mfct.getBlock(block.id());

    for (ir::BlockId pred: block.preds()) {
        if (reachable[pred]) {
            mblock.preds().push_back(pred);
        }
    }

    for (const ir::Instruction& inst: block.instrs()) {
        // If this is the last intruction of the IR block, we will resolve phis by injecting MOV
        // At the end of the block by looking at the block successor's phis
        if (isTerminator(inst.op())) {
            for (ir::BlockId succ: block.successors()) {
                for (const ir::Phi& phi: fct.getBlock(succ).phis()) {
                    for (const auto& [id, var]: phi.m_incomings) {
                        if (id == block.id()) {
                            // i64 %1 = PHI [entry: i64 1], [if.then: i64 20]
                            // Becomes %1 = 1 at then end of entry block (before terminator)
                            // i64 %2 = PHI [entry: i64 0], [if.then: i64 10]
                            // Becomes %2 = 0 at then end of entry block (before terminator)
                            // Then block will also have those revoled then encountered later
                            ir::Instruction phiresolve(phi.m_result, var, ir::NONE(), ir::Opcode::COPY);
                            this->lowerInstruction(mfct, mblock, phiresolve);
                        }
                    }
                }
            }
        }
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
        /*
        a > b -> a - b > 0 -> cmp a, b then following instructions will use those flags
        If is usualy followed by jg and jmp: https://www.aldeid.com/wiki/X86-assembly/Instructions/jg
        if (a > b) { TOTO } TATA
        cmp b, a
        jg TOTO ---> If flag goto TOTO, if not continue
        JMP TATA

        cmp a, b computes a - b, throws the result away, and sets four flags: 
        ZF (zero, the result was 0), 
        SF (sign, the result's top bit is 1), 
        OF (overflow, the signed subtraction overflowed), 
        CF (carry, the unsigned subtraction borrowed).
        */
        case ir::Opcode::GT:
            mblock.addInstruction(MachineOpcode::MOV, dst, lhs);
            mblock.addInstruction(MachineOpcode::CMP, dst, rhs);
            break;
        case ir::Opcode::CBR:
            // CBR %v L1 L2 becomes
            // jg L1
            // jmp L2
            // TODO: JG is hardcoded and the flags are assumed to come from the
            // instruction right before. Track which comparison defines
            // inst.result() and pick the suffix from its opcode.
            mblock.addInstruction(MachineOpcode::JG, lhs, MNONE());
            mblock.addInstruction(MachineOpcode::JMP, rhs, MNONE());
            break;
        case ir::Opcode::RET:
            // The returned value sits in m_src so liveness reads it as a use:
            // as a destination it would look like a write and kill the value.
            mblock.addInstruction(MachineOpcode::RET, MNONE(), lhs);
            break;
        // Unconditionnal jump
        case ir::Opcode::BR:
            mblock.addInstruction(MachineOpcode::JMP, lhs, MNONE());
            break;
        // Simple two-adress copy
        case ir::Opcode::COPY:
            mblock.addInstruction(MachineOpcode::MOV, dst, lhs);
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
        case ir::Operand::Kind::BLOCK:
            // A jump target is just the label of the block it was lowered to.
            return SYM(mfct.getBlock(op.m_block).label(), op.m_type);
        default:
            throw CodegenError("[X86] Unsupported operand kind");
    }
}


void X86::addRegister(Register r) {
    this->m_registers.push_back(std::move(r));
    // The key views the label stored in the vector, not the moved-from argument
    const Register& stored = this->m_registers.back();
    this->m_regnames[stored.label()] = stored.id();
}

ir::ValueId X86::getRegId(const std::string& name) {
    const auto it = this->m_regnames.find(name);
    if (it == this->m_regnames.end()) {
        throw CodegenError("[X86] Unknown register '" + name + "'");
    }
    return it->second;
}

const Register& X86::getReg(const std::string& name) {
    return this->m_registers[this->getRegId(name)];
}

const std::set<ir::ValueId>& X86::family(ir::ValueId id) const {
    const auto it = this->m_families.find(id);
    if (it == this->m_families.end()) {
        throw CodegenError("[X86] Unknown register id " + std::to_string(id));
    }
    return it->second;
}

X86::X86(): m_registers(), m_families(), m_regnames() {
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
    this->addRegister(Register(0, "NONE", ir::VOID(), RegClass::GP, 0, false, false));
    this->addRegister(std::move(RAX));
    this->addRegister(std::move(EAX));
    this->addRegister(std::move(AX));
    this->addRegister(std::move(AH));
    this->addRegister(std::move(AL));
    this->addRegister(std::move(RCX));
    this->addRegister(std::move(ECX));
    this->addRegister(std::move(CX));
    this->addRegister(std::move(CH));
    this->addRegister(std::move(CL));
    this->addRegister(std::move(RDX));
    this->addRegister(std::move(EDX));
    this->addRegister(std::move(DX));
    this->addRegister(std::move(DH));
    this->addRegister(std::move(DL));
    this->addRegister(std::move(RBX));
    this->addRegister(std::move(EBX));
    this->addRegister(std::move(BX));
    this->addRegister(std::move(BH));
    this->addRegister(std::move(BL));
    this->addRegister(std::move(RSP));
    this->addRegister(std::move(ESP));
    this->addRegister(std::move(SP));
    this->addRegister(std::move(SPL));
    this->addRegister(std::move(RBP));
    this->addRegister(std::move(EBP));
    this->addRegister(std::move(BP));
    this->addRegister(std::move(BPL));
    this->addRegister(std::move(RSI));
    this->addRegister(std::move(ESI));
    this->addRegister(std::move(SI));
    this->addRegister(std::move(SIL));
    this->addRegister(std::move(RDI));
    this->addRegister(std::move(EDI));
    this->addRegister(std::move(DI));
    this->addRegister(std::move(DIL));
    this->addRegister(std::move(R8));
    this->addRegister(std::move(R8D));
    this->addRegister(std::move(R8W));
    this->addRegister(std::move(R8B));
    this->addRegister(std::move(R9));
    this->addRegister(std::move(R9D));
    this->addRegister(std::move(R9W));
    this->addRegister(std::move(R9B));
    this->addRegister(std::move(R10));
    this->addRegister(std::move(R10D));
    this->addRegister(std::move(R10W));
    this->addRegister(std::move(R10B));
    this->addRegister(std::move(R11));
    this->addRegister(std::move(R11D));
    this->addRegister(std::move(R11W));
    this->addRegister(std::move(R11B));
    this->addRegister(std::move(R12));
    this->addRegister(std::move(R12D));
    this->addRegister(std::move(R12W));
    this->addRegister(std::move(R12B));
    this->addRegister(std::move(R13));
    this->addRegister(std::move(R13D));
    this->addRegister(std::move(R13W));
    this->addRegister(std::move(R13B));
    this->addRegister(std::move(R14));
    this->addRegister(std::move(R14D));
    this->addRegister(std::move(R14W));
    this->addRegister(std::move(R14B));
    this->addRegister(std::move(R15));
    this->addRegister(std::move(R15D));
    this->addRegister(std::move(R15W));
    this->addRegister(std::move(R15B));
    this->addRegister(std::move(ZMM0));
    this->addRegister(std::move(YMM0));
    this->addRegister(std::move(XMM0));
    this->addRegister(std::move(ZMM1));
    this->addRegister(std::move(YMM1));
    this->addRegister(std::move(XMM1));
    this->addRegister(std::move(ZMM2));
    this->addRegister(std::move(YMM2));
    this->addRegister(std::move(XMM2));
    this->addRegister(std::move(ZMM3));
    this->addRegister(std::move(YMM3));
    this->addRegister(std::move(XMM3));
    this->addRegister(std::move(ZMM4));
    this->addRegister(std::move(YMM4));
    this->addRegister(std::move(XMM4));
    this->addRegister(std::move(ZMM5));
    this->addRegister(std::move(YMM5));
    this->addRegister(std::move(XMM5));
    this->addRegister(std::move(ZMM6));
    this->addRegister(std::move(YMM6));
    this->addRegister(std::move(XMM6));
    this->addRegister(std::move(ZMM7));
    this->addRegister(std::move(YMM7));
    this->addRegister(std::move(XMM7));
    this->addRegister(std::move(ZMM8));
    this->addRegister(std::move(YMM8));
    this->addRegister(std::move(XMM8));
    this->addRegister(std::move(ZMM9));
    this->addRegister(std::move(YMM9));
    this->addRegister(std::move(XMM9));
    this->addRegister(std::move(ZMM10));
    this->addRegister(std::move(YMM10));
    this->addRegister(std::move(XMM10));
    this->addRegister(std::move(ZMM11));
    this->addRegister(std::move(YMM11));
    this->addRegister(std::move(XMM11));
    this->addRegister(std::move(ZMM12));
    this->addRegister(std::move(YMM12));
    this->addRegister(std::move(XMM12));
    this->addRegister(std::move(ZMM13));
    this->addRegister(std::move(YMM13));
    this->addRegister(std::move(XMM13));
    this->addRegister(std::move(ZMM14));
    this->addRegister(std::move(YMM14));
    this->addRegister(std::move(XMM14));
    this->addRegister(std::move(ZMM15));
    this->addRegister(std::move(YMM15));
    this->addRegister(std::move(XMM15));
    this->addRegister(std::move(K0));
    this->addRegister(std::move(K1));
    this->addRegister(std::move(K2));
    this->addRegister(std::move(K3));
    this->addRegister(std::move(K4));
    this->addRegister(std::move(K5));
    this->addRegister(std::move(K6));
    this->addRegister(std::move(K7));

    // Compute famillies (repondant information but easy to look into)
    for (const Register& reg: this->registers()) {
        this->m_families[reg.id()].insert(reg.id());
        this->m_families[reg.id()].insert(reg.parent());
        this->m_families[reg.id()].insert(reg.subregs().begin(), reg.subregs().end());
    }
}


void allocate(MachineFunction& fct, X86& x86);

RegisterAllocator::RegisterAllocator(X86& x86) {
    // Building successors
    for (MachineFunction& f: x86.fcts()){
        allocate(f, x86);
    }
}


// https://cse.sc.edu/~mgv/csce531sp20/notes/mogensen_Ch8_Slides_register-allocation.pdf
void allocate(MachineFunction& fct, X86& x86) {
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
            for (const Register& r: x86.registers()) {
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
        for(const Register& r: x86.registers()) {
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
            // TODO: Spill

        } else {
            // Color the node with it first available color
            colors[*it] = *domains.at(*it).begin();
            // Tackling aliasing:
            // If we pick RAX, then EAX, AX can NOT be picked either in the interference
            // We will remove domains.at(*it) and its familly from domains
            for (ir::ValueId color: x86.family(*domains.at(*it).begin())) {
                // This color can then be removed from the node neighboor's domain
                for(ir::ValueId n: inter_graph.at(*it)) {
                    domains.at(n).erase(color);
                }
            }
            // This node that then be removed from the interferance graph
            for (ir::ValueId n: inter_graph.at(*it)) {
                inter_graph.at(n).erase(*it);
            }
            inter_graph.erase(*it);
        }
    } while (!inter_graph.empty());

    // Apply the coloring by changing VREG into PREG!
    for (auto it = order.begin(); it != order.end(); ++it) {
        MachineInstruction* i = *it;
        if (i->m_src.m_kind == MachineOperand::Kind::VREG) {
            i->m_src = PREG(&x86.registers()[colors.at(regId(i->m_src))], i->m_src.m_type);
        }
        if (i->m_dst.m_kind == MachineOperand::Kind::VREG) {
            i->m_dst = PREG(&x86.registers()[colors.at(regId(i->m_dst))], i->m_dst.m_type);
        }
    }
}



} // namespace blast::core::codegen
