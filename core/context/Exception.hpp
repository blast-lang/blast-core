#pragma once
#include <core/Exception.hpp>
#include <core/context/Scope.hpp>
#include <core/context/Type.hpp>
#include <core/parser/Ast.hpp>

namespace blast::core::context {

class UndeclaredIdentifier: public SemanticError {
public:
    UndeclaredIdentifier(std::string name, parser::NodeId node):
        SemanticError("Identifier '" + name + "' is not declared in this scope"),
        m_name(std::move(name)), m_node(node) {}

    const std::string& name() const { return m_name; }
    parser::NodeId node() const { return m_node; }

private:
    std::string m_name;
    parser::NodeId m_node;
};

class Redeclaration: public SemanticError {
public:
    Redeclaration(std::string name, parser::NodeId node):
        SemanticError("'" + name + "' is already declared in this scope"),
        m_name(std::move(name)), m_node(node) {}

    const std::string& name() const { return m_name; }
    parser::NodeId node() const { return m_node; }

private:
    std::string m_name;
    parser::NodeId m_node;
};

class NotAFunction: public SemanticError {
public:
    NotAFunction(const Symbol* existing, parser::NodeId node):
        SemanticError("'" + std::string(existing->name()) + "' is already declared in this scope and is not a function"),
        m_existing(existing), m_node(node) {}

    const Symbol* existing() const { return m_existing; }
    parser::NodeId node() const { return m_node; }

private:
    const Symbol* m_existing;
    parser::NodeId m_node;
};

class NotAModule: public SemanticError {
public:
    NotAModule(const Symbol* base, parser::NodeId node):
        SemanticError("'" + std::string(base->name()) + "' is not a module"),
        m_base(base), m_node(node) {}

    const Symbol* base() const { return m_base; }
    parser::NodeId node() const { return m_node; }

private:
    const Symbol* m_base;
    parser::NodeId m_node;
};

class UnknownMember: public SemanticError {
public:
    UnknownMember(std::string member, const Symbol* module, parser::NodeId node):
        SemanticError("Unknown member '" + member + "' of module '" + std::string(module->name()) + "'"),
        m_member(std::move(member)), m_module(module), m_node(node) {}

    const std::string& member() const { return m_member; }
    const Symbol* module() const { return m_module; }
    parser::NodeId node() const { return m_node; }

private:
    std::string m_member;
    const Symbol* m_module;
    parser::NodeId m_node;
};

class OperandMismatch: public SemanticError {
public:
    OperandMismatch(std::string op, const Type* lhs, const Type* rhs, parser::NodeId node):
        SemanticError("Operator '" + op + "' has operands of different types '" + lhs->name() + "' and '" + rhs->name() + "'"),
        m_op(std::move(op)), m_lhs(lhs), m_rhs(rhs), m_node(node) {}

    const std::string& op() const { return m_op; }
    const Type* lhs() const { return m_lhs; }
    const Type* rhs() const { return m_rhs; }
    parser::NodeId node() const { return m_node; }

private:
    std::string m_op;
    const Type* m_lhs;
    const Type* m_rhs;
    parser::NodeId m_node;
};

class TypeMismatch: public SemanticError {
public:
    TypeMismatch(const Type* expected, const Type* actual, parser::NodeId node):
        SemanticError("Expected type '" + expected->name() + "' but got '" + actual->name() + "'"),
        m_expected(expected), m_actual(actual), m_node(node) {}

    const Type* expected() const { return m_expected; }
    const Type* actual() const { return m_actual; }
    parser::NodeId node() const { return m_node; }

private:
    const Type* m_expected;
    const Type* m_actual;
    parser::NodeId m_node;
};

class ConflictingReturnTypes: public SemanticError {
public:
    ConflictingReturnTypes(const Type* first, const Type* second):
        SemanticError("Paths return different types '" + first->name() + "' and '" + second->name() + "'"),
        m_first(first), m_second(second) {}

    const Type* first() const { return m_first; }
    const Type* second() const { return m_second; }

private:
    const Type* m_first;
    const Type* m_second;
};

class UndefinedParameterType: public SemanticError {
public:
    UndefinedParameterType(const parser::VarDecl& param, const parser::FctDecl& function):
        SemanticError("Parameter '" + param.name() + "' of '" + function.name() + "' has an undefined type"),
        m_param(param.name()), m_function(function.name()), m_node(param.id()) {}

    const std::string& param() const { return m_param; }
    const std::string& function() const { return m_function; }
    parser::NodeId node() const { return m_node; }

private:
    std::string m_param;
    std::string m_function;
    parser::NodeId m_node;
};

class UndefinedReturnType: public SemanticError {
public:
    explicit UndefinedReturnType(const parser::FctDecl& function):
        SemanticError("Function '" + function.name() + "' has an undefined return type"),
        m_function(function.name()), m_node(function.id()) {}

    const std::string& function() const { return m_function; }
    parser::NodeId node() const { return m_node; }

private:
    std::string m_function;
    parser::NodeId m_node;
};

class DuplicateMethod: public SemanticError {
public:
    DuplicateMethod(const parser::FctDecl& function, const TupleType* args):
        SemanticError("Function '" + function.name() + "' is already defined for '" + args->name() + "'"),
        m_function(function.name()), m_args(args), m_node(function.id()) {}

    const std::string& function() const { return m_function; }
    const TupleType* args() const { return m_args; }
    parser::NodeId node() const { return m_node; }

private:
    std::string m_function;
    const TupleType* m_args;
    parser::NodeId m_node;
};

class UnresolvedMethod: public SemanticError {
public:
    explicit UnresolvedMethod(const parser::FctDecl& function):
        SemanticError("Function '" + function.name() + "' has no registered method"),
        m_function(function.name()), m_node(function.id()) {}

    const std::string& function() const { return m_function; }
    parser::NodeId node() const { return m_node; }

private:
    std::string m_function;
    parser::NodeId m_node;
};

} // namespace blast::core::context
