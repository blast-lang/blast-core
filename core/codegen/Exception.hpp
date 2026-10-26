#pragma once
#include <core/Exception.hpp>
#include <core/codegen/X86.hpp>

namespace blast::core::codegen {

class InvalidBranchCondition: public CodegenError {
public:
    explicit InvalidBranchCondition(ir::Operand condition):
        CodegenError("[X86] CBR condition is not a comparison"), m_condition(condition) {}

    const ir::Operand& condition() const { return m_condition; }

private:
    ir::Operand m_condition;
};

class UnsupportedComparison: public CodegenError {
public:
    explicit UnsupportedComparison(ir::Opcode opcode):
        CodegenError("[X86] Unknown comparison (opcode " + std::to_string((int)opcode) + ")"), m_opcode(opcode) {}

    ir::Opcode opcode() const { return m_opcode; }

private:
    ir::Opcode m_opcode;
};

class UnsupportedOpcode: public CodegenError {
public:
    explicit UnsupportedOpcode(ir::Opcode opcode):
        CodegenError("[X86] Unsupported opcode " + std::to_string((int)opcode)), m_opcode(opcode) {}

    ir::Opcode opcode() const { return m_opcode; }

private:
    ir::Opcode m_opcode;
};

class UnsupportedOperand: public CodegenError {
public:
    explicit UnsupportedOperand(ir::Operand operand):
        CodegenError("[X86] Unsupported operand kind " + std::to_string((int)operand.m_kind)), m_operand(operand) {}

    const ir::Operand& operand() const { return m_operand; }

private:
    ir::Operand m_operand;
};

class UnknownMachineOpcode: public CodegenError {
public:
    explicit UnknownMachineOpcode(MachineOpcode opcode):
        CodegenError("[emit] Unknown opcode " + std::to_string((int)opcode)), m_opcode(opcode) {}

    MachineOpcode opcode() const { return m_opcode; }

private:
    MachineOpcode m_opcode;
};

class UnsupportedMachineOperand: public CodegenError {
public:
    explicit UnsupportedMachineOperand(MachineOperand operand):
        CodegenError("[emit] Unsupported operand kind " + std::to_string((int)operand.m_kind) + " at emit time"), m_operand(operand) {}

    const MachineOperand& operand() const { return m_operand; }

private:
    MachineOperand m_operand;
};

class MissingRegister: public CodegenError {
public:
    explicit MissingRegister(MachineOperand operand):
        CodegenError("[emit] Physical register operand without a register"), m_operand(operand) {}

    const MachineOperand& operand() const { return m_operand; }

private:
    MachineOperand m_operand;
};

class UnallocatedRegister: public CodegenError {
public:
    explicit UnallocatedRegister(MachineOperand operand):
        CodegenError("[emit] Virtual register %" + std::to_string(operand.m_vreg) + " left at emit time"), m_operand(operand) {}

    const MachineOperand& operand() const { return m_operand; }

private:
    MachineOperand m_operand;
};

class UnsupportedWidth: public CodegenError {
public:
    explicit UnsupportedWidth(ir::Type type):
        CodegenError("[emit] Unsupported width " + std::to_string(ir::bits(type))), m_type(type) {}

    ir::Type type() const { return m_type; }

private:
    ir::Type m_type;
};

class UnknownRegister: public CodegenError {
public:
    explicit UnknownRegister(std::string name):
        CodegenError("[X86] Unknown register '" + name + "'"), m_name(std::move(name)) {}

    const std::string& name() const { return m_name; }

private:
    std::string m_name;
};

class UnknownRegisterId: public CodegenError {
public:
    explicit UnknownRegisterId(ir::ValueId id):
        CodegenError("[X86] Unknown register id " + std::to_string(id)), m_id(id) {}

    ir::ValueId id() const { return m_id; }

private:
    ir::ValueId m_id;
};

class UnknownBlock: public CodegenError {
public:
    UnknownBlock(ir::BlockId id, std::string function):
        CodegenError("[MachineFunction] Unknown block id " + std::to_string(id) + " in '" + function + "'"),
        m_id(id), m_function(std::move(function)) {}

    ir::BlockId id() const { return m_id; }
    const std::string& function() const { return m_function; }

private:
    ir::BlockId m_id;
    std::string m_function;
};

} // namespace blast::core::codegen
