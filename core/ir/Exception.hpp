#pragma once
#include <core/Exception.hpp>
#include <core/ir/IR.hpp>

namespace blast::core::ir {

class UnsupportedOperator: public IRError {
public:
    explicit UnsupportedOperator(std::string op):
        IRError("Unsupported operator '" + op + "'"), m_op(std::move(op)) {}

    const std::string& op() const { return m_op; }

private:
    std::string m_op;
};

// An AST node the front end accepts but the lowering does not handle yet.
class UnsupportedConstruct: public IRError {
public:
    explicit UnsupportedConstruct(const parser::ASTNode& node):
        IRError("AST node kind " + std::to_string((int)node.kind()) + " (node " + std::to_string(node.id()) + ") is not lowered yet", node.location()),
        m_kind(node.kind()), m_node(node.id()) {}

    parser::ASTNode::Kind kind() const { return m_kind; }
    parser::NodeId node() const { return m_node; }

private:
    parser::ASTNode::Kind m_kind;
    parser::NodeId m_node;
};

class NotCallable: public IRError {
public:
    explicit NotCallable(const parser::ASTNode& callee):
        IRError("[IR] Callee (node " + std::to_string(callee.id()) + ") is not a function", callee.location()),
        m_callee(callee.id()) {}

    parser::NodeId callee() const { return m_callee; }

private:
    parser::NodeId m_callee;
};

class MissingMethod: public IRError {
public:
    explicit MissingMethod(const parser::FctDecl& decl):
        IRError("[IR] Function '" + decl.name() + "' has no method", decl.location()), m_name(decl.name()), m_node(decl.id()) {}

    const std::string& name() const { return m_name; }
    parser::NodeId node() const { return m_node; }

private:
    std::string m_name;
    parser::NodeId m_node;
};

class MissingFunctionType: public IRError {
public:
    explicit MissingFunctionType(const context::Method& method):
        IRError("[IR] Function '" + method.name + "' has no function type"), m_name(method.name) {}

    const std::string& name() const { return m_name; }

private:
    std::string m_name;
};

class NonPrimitiveType: public IRError {
public:
    explicit NonPrimitiveType(const context::Type* type):
        IRError("[IR] Cannot lower non-primitive type" + (type == nullptr ? std::string() : " '" + type->name() + "'")),
        m_type(type) {}

    const context::Type* type() const { return m_type; }

private:
    const context::Type* m_type;
};

class UnsupportedType: public IRError {
public:
    explicit UnsupportedType(const context::Type* type):
        IRError("[IR] Unsupported type '" + type->name() + "'"), m_type(type) {}

    const context::Type* type() const { return m_type; }

private:
    const context::Type* m_type;
};

class UnknownIRBlock: public IRError {
public:
    UnknownIRBlock(BlockId block, std::string function):
        IRError("[Function] Unknown block id " + std::to_string(block) + " in '" + function + "'"),
        m_block(block), m_function(std::move(function)) {}

    BlockId block() const { return m_block; }
    const std::string& function() const { return m_function; }

private:
    BlockId m_block;
    std::string m_function;
};

class NotACall: public IRError {
public:
    NotACall(Opcode opcode, std::string function):
        IRError("[Function] Instruction (opcode " + std::to_string((int)opcode) + ") is not a call in '" + function + "'"),
        m_opcode(opcode), m_function(std::move(function)) {}

    Opcode opcode() const { return m_opcode; }
    const std::string& function() const { return m_function; }

private:
    Opcode m_opcode;
    std::string m_function;
};

class UnknownArgumentList: public IRError {
public:
    UnknownArgumentList(std::uint32_t index, std::string function):
        IRError("[Function] Unknown argument list " + std::to_string(index) + " in '" + function + "'"),
        m_index(index), m_function(std::move(function)) {}

    std::uint32_t index() const { return m_index; }
    const std::string& function() const { return m_function; }

private:
    std::uint32_t m_index;
    std::string m_function;
};

class UnknownFunction: public IRError {
public:
    explicit UnknownFunction(FctId function):
        IRError("[Module] Unknown function id " + std::to_string(function)), m_function(function) {}

    FctId function() const { return m_function; }

private:
    FctId m_function;
};

class MissingTerminator: public IRError {
public:
    explicit MissingTerminator(const BasicBlock& block):
        IRError("[BasicBlock] Block '" + block.label() + "' does not end in a terminator"),
        m_block(block.id()), m_label(block.label()) {}

    BlockId block() const { return m_block; }
    const std::string& label() const { return m_label; }

private:
    BlockId m_block;
    std::string m_label;
};

} // namespace blast::core::ir
