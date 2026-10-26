#pragma once
#include <core/Exception.hpp>
#include <core/ir/IR.hpp>

namespace blast::core::ir {

class UnsupportedOperator : public IRError {
public:
    explicit UnsupportedOperator(std::string op):
        IRError("Unsupported operator '" + op + "'"), m_op(std::move(op)) {}

    const std::string& op() const { return m_op; }

private:
    std::string m_op;
};

// An AST node the front end accepts but the lowering does not handle yet.
class UnsupportedConstruct : public IRError {
public:
    UnsupportedConstruct(parser::ASTNode::Kind kind, parser::NodeId node):
        IRError("AST node kind " + std::to_string((int)kind) + " (node " + std::to_string(node) + ") is not lowered yet"),
        m_kind(kind), m_node(node) {}

    parser::ASTNode::Kind kind() const { return m_kind; }
    parser::NodeId node() const { return m_node; }

private:
    parser::ASTNode::Kind m_kind;
    parser::NodeId m_node;
};

class NotCallable : public IRError {
public:
    explicit NotCallable(parser::NodeId callee):
        IRError("[IR] Callee (node " + std::to_string(callee) + ") is not a function"), m_callee(callee) {}

    parser::NodeId callee() const { return m_callee; }

private:
    parser::NodeId m_callee;
};

class MissingMethod : public IRError {
public:
    explicit MissingMethod(const parser::FctDecl& decl):
        IRError("[IR] Function '" + decl.name() + "' has no method"), m_name(decl.name()), m_node(decl.id()) {}

    const std::string& name() const { return m_name; }
    parser::NodeId node() const { return m_node; }

private:
    std::string m_name;
    parser::NodeId m_node;
};

class MissingFunctionType : public IRError {
public:
    explicit MissingFunctionType(const context::Method& method):
        IRError("[IR] Function '" + method.name + "' has no function type"), m_name(method.name) {}

    const std::string& name() const { return m_name; }

private:
    std::string m_name;
};

class MissingTerminator : public IRError {
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
