#include <core/ir/IR.hpp>
#include <core/Exception.hpp>

namespace blast::core::ir {

namespace {

Opcode opcodeFor(const std::string& op) {
    if (op == "+") return Opcode::ADD;
    if (op == "*") return Opcode::MUL;
    throw CodegenError("unsupported operator '" + op + "'");
}

} // namespace


Operand SSAIR::visitIntLiteral(const parser::IntLiteral& node) {
    return INT_LIT(node.value());
}

// Operand's LITERAL payload is an int64, so there is nowhere to put these yet.
// Fail loudly rather than lower them to something they are not: a silent
// visitEmpty() here is indistinguishable from a correctly empty block.
Operand SSAIR::visitFloatLiteral(const parser::FloatLiteral&) {
    throw CodegenError("float literals are not lowered yet");
}

Operand SSAIR::visitBoolLiteral(const parser::BoolLiteral&) {
    throw CodegenError("bool literals are not lowered yet");
}

Operand SSAIR::visitStringLiteral(const parser::StringLiteral&) {
    throw CodegenError("string literals are not lowered yet");
}

Operand SSAIR::visitIdentifier(const parser::Identifier& node) {
    return NONE();
}

Operand SSAIR::visitBinaryExpr(const parser::BinaryExpr& node) {
    const Operand lhs = this->visit(node.lhs());
    const Operand rhs = this->visit(node.rhs());
    return this->currentBlock().newInstruction(lhs, rhs, opcodeFor(node.op()));
}

Operand SSAIR::visitVarDecl(const parser::VarDecl& node) {
    return NONE();
}

Operand SSAIR::visitAssign(const parser::Assign& node) {
    return NONE();
}

Operand SSAIR::visitExprStmt(const parser::ExprStmt& node) {
    return this->visit(node.expr());
}

Operand SSAIR::visitTranslationUnit(const parser::TranslationUnit& node) {
    for (const auto& stmt : node.stmts()) {
        this->visit(stmt.get());
    }
    
    // A translation unit is not an expression: it has no value.
    return NONE();
}

Function SSAIR::run(const parser::TranslationUnit& unit) {
    this->visit(&unit);
    return this->currentFct();
}

} // namespace blast::core::ir
