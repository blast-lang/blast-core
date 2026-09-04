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

MachineIR::MachineIR(const ir::Module& m) {

}

} // namespace blast::core::codegen
