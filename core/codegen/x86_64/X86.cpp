#include <core/codegen/x86_64/X86.hpp>

namespace blast::core::codegen::x86_64 {

const char* mnemonic(TargetOpcode op) {
    switch (static_cast<Opcode>(op)) {
        case Opcode::MOV:      return "MOV";
        case Opcode::ADD:      return "ADD";
        case Opcode::SUB:      return "SUB";
        case Opcode::IMUL:     return "IMUL";
        case Opcode::CQO:      return "CQO";
        case Opcode::DIV:      return "DIV";
        case Opcode::IDIV:     return "IDIV";
        case Opcode::NEG:      return "NEG";
        case Opcode::CMP:      return "CMP";
        case Opcode::SETCC:    return "SETCC";
        case Opcode::JMP:      return "JMP";
        case Opcode::JCC:      return "JCC";
        case Opcode::CALL:     return "CALL";
        case Opcode::RET:      return "RET";
        case Opcode::PROLOGUE: return "PROLOGUE";
        case Opcode::EPILOGUE: return "EPILOGUE";
    }
    return "?";
}

Width width(ir::Type t) {
    switch (t) {
        case ir::Type::I1:   return Width::W8;
        case ir::Type::I8:   return Width::W8;
        case ir::Type::UI8:  return Width::W8;

        case ir::Type::I16:  return Width::W16;
        case ir::Type::UI16: return Width::W16;

        case ir::Type::I32:  return Width::W32;
        case ir::Type::UI32: return Width::W32;
        case ir::Type::F32:  return Width::W32;

        case ir::Type::I64:  return Width::W64;
        case ir::Type::UI64: return Width::W64;
        case ir::Type::F64:  return Width::W64;

        case ir::Type::PTR:  return Width::W64;
        case ir::Type::VOID: return Width::W64;
    }
    return Width::W64;
}

bool isSigned(ir::Type t) {
    return ir::isInt(t) && t != ir::Type::I1;
}

RegClass regClass(ir::Type t) {
    return ir::isFloat(t) ? RegClass::FLOAT : RegClass::INT;
}

namespace {

constexpr VRegId NO_VREG = static_cast<VRegId>(-1);

// ir value ids are dense per function, so a vector is enough of a map.
VRegId vregOf(MachineFunction& mf, std::vector<VRegId>& map, const ir::Operand& op) {
    if (op.m_value >= map.size()) {
        map.resize(op.m_value + 1, NO_VREG);
    }
    if (map[op.m_value] == NO_VREG) {
        map[op.m_value] = mf.newVReg(regClass(op.m_type));
    }
    return map[op.m_value];
}

MachineOperand operandOf(MachineFunction& mf, std::vector<VRegId>& map, const ir::Operand& op, Role r) {
    switch (op.m_kind) {
        case ir::Operand::Kind::REGISTER: return MachineOperand::vreg(vregOf(mf, map, op), r);
        case ir::Operand::Kind::LITERAL:  return MachineOperand::imm(op.m_lit);
        case ir::Operand::Kind::BLOCK:    return MachineOperand::label(op.m_block);
        case ir::Operand::Kind::NONE:     return MachineOperand::none();
    }
    return MachineOperand::none();
}

TargetOpcode arithOpcode(ir::Opcode o, ir::Type t) {
    switch (o) {
        case ir::Opcode::ADD: return opcode(Opcode::ADD);
        case ir::Opcode::SUB: return opcode(Opcode::SUB);
        case ir::Opcode::MUL: return opcode(Opcode::IMUL);
        case ir::Opcode::DIV: return opcode(isSigned(t) ? Opcode::IDIV : Opcode::DIV);
        default: break;
    }
    return opcode(Opcode::MOV);
}

// Three-address into two: %0 = MUL 6, 7 becomes MOV %0, 6 then IMUL %0, 7.
void lowerInstr(MachineFunction& mf, MachineBlock& mb, std::vector<VRegId>& map, const ir::Instruction& i) {
    switch (i.op()) {
        case ir::Opcode::ADD:
        case ir::Opcode::SUB:
        case ir::Opcode::MUL: {
            const Width w = width(i.result().m_type);
            mb.addInstruction(MachineInstr(opcode(Opcode::MOV), w, {
                operandOf(mf, map, i.result(), Role::DEF),
                operandOf(mf, map, i.lhs(), Role::USE),
            }));
            mb.addInstruction(MachineInstr(arithOpcode(i.op(), i.result().m_type), w, {
                operandOf(mf, map, i.result(), Role::DEF_USE),
                operandOf(mf, map, i.rhs(), Role::USE),
            }));
            break;
        }
        default: break;
    }
}

} // namespace

MachineIR lower(const ir::Module& m) {
    MachineIR mir(&mnemonic);

    for (const ir::Function& f : m.fcts()) {
        MachineFunction& mf = mir.addFct(f.name());
        std::vector<VRegId> map;

        for (const ir::BasicBlock& b : f.blocks()) {
            mf.addBlock(b.label());
        }

        for (const ir::BasicBlock& b : f.blocks()) {
            MachineBlock& mb = mf.getBlock(b.id());
            for (const ir::Instruction& i : b.instrs()) {
                lowerInstr(mf, mb, map, i);
            }
            for (ir::BlockId p : b.preds()) {
                mf.addEdge(p, b.id());
            }
        }
    }

    return mir;
}

} // namespace blast::core::codegen::x86_64
