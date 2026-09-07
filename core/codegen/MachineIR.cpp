#include <core/codegen/MachineIR.hpp>

namespace blast::core::codegen {

Width width(const ir::Operand& op) {
    switch(op.m_type) {
        case ir::Type::I8:  return Width::W8;
        case ir::Type::UI8:  return Width::W8;

        case ir::Type::I16:  return Width::W16;
        case ir::Type::UI16:  return Width::W16;

        case ir::Type::I32:  return Width::W32;
        case ir::Type::UI32:  return Width::W32;
        case ir::Type::F32:  return Width::W32;

        case ir::Type::I64:  return Width::W64;
        case ir::Type::UI64:  return Width::W64;
        case ir::Type::F64:  return Width::W64;

        case ir::Type::PTR:  return Width::W64;
        case ir::Type::VOID:  return Width::W64;

        default: return Width::W8;
    }
}

bool isSigned(ir::Type t) {
    switch(t) {
        case ir::Type::I8:  return true;
        case ir::Type::I16:  return true;
        case ir::Type::I32:  return true;
        case ir::Type::I64:  return true;

        case ir::Type::I1:  return false;
        case ir::Type::UI8:  return false;
        case ir::Type::UI16:  return false;
        case ir::Type::UI32:  return false;
        case ir::Type::UI64:  return false;

        // Floats have their own opcodes, they never select on signedness
        case ir::Type::F32:  return false;
        case ir::Type::F64:  return false;

        case ir::Type::PTR:  return false;
        case ir::Type::VOID:  return false;

        default: return false;
    }
}

MachineOperand::MachineOperand(const ir::Operand& op) {
    switch(op.m_kind) {
        case ir::Operand::Kind::NONE: {
            // throw
            break;
        }
        case ir::Operand::Kind::BLOCK: {
            this->m_kind = MachineOperand::Kind::LABEL;
            this->m_block = op.m_block;
            break;
        }
        case ir::Operand::Kind::REGISTER: {
            this->m_kind = MachineOperand::Kind::VREG;
            this->m_value = op.m_value;
            break;
        }
        case ir::Operand::Kind::LITERAL: {
            this->m_kind = MachineOperand::Kind::LITERAL;
            this->m_lit = op.m_lit;
            break;
        }
    }
}

MachineOpcode opcode(ir::Opcode o, const ir::Operand& op) {
    switch(o) {
        case ir::Opcode::ADD: return MachineOpcode::ADD;
        case ir::Opcode::MUL: return MachineOpcode::IMUL;
        case ir::Opcode::DIV: return (isSigned(op.m_type) ? MachineOpcode::IDIV: MachineOpcode::DIV);
    }
    return MachineOpcode::RET;
}

void MachineBlock::addInstruction(const ir::Instruction& i) {
    // Lowering Three-Address Code into Two Adresses
    // i64 %0 = MUL i64 6, i64 7:
    // MOV  W64  vreg0, 6
    // IMUL W64  vreg0, 7
    if (
        i.op() == ir::Opcode::ADD ||
        i.op() == ir::Opcode::MUL
    ) {
        this->m_instrs.push_back(MachineInstr(
            MachineOpcode::MOV, width(i.result()), MachineOperand(i.result()), MachineOperand(i.lhs())
        ));
        this->m_instrs.push_back(MachineInstr(
            opcode(i.op(), i.result()), width(i.result()), MachineOperand(i.result()), MachineOperand(i.rhs())
        ));
    }
}

void MachineIR::addBlock(const ir::BasicBlock& b) {
    this->m_blocks.push_back(MachineBlock(b.id(), b.label()));
    for (const ir::Instruction& i: b.instrs()) {
        this->m_blocks[this->m_blocks.size()-1].addInstruction(i);
    }
}


MachineIR::MachineIR(const ir::Module& m): m_blocks() {
    for (const ir::Function& f: m.fcts()) {
        for (const ir::BasicBlock& b: f.blocks()) {
            this->addBlock(b);
        }
    }
}

} // namespace blast::core::codegen
