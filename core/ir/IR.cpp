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
    // Get the associated symbols's register (if it exist)
    return this->getLKO(this->m_ctx->getNodeSymbol(&node));
}

Operand SSAIR::visitBinaryExpr(const parser::BinaryExpr& node) {
    const Operand lhs = this->visit(node.lhs());
    const Operand rhs = this->visit(node.rhs());
    return this->addInstruction(lhs, rhs, opcodeFor(node.op()));
}

Operand SSAIR::visitVarDecl(const parser::VarDecl& node) {
    Operand reg;
    context::Symbol* v = this->m_ctx->getNodeSymbol(&node);
    if (node.hasInit()) {
        reg = this->visit(node.init());
    } else {
        // Get default assign (0 for int)
        reg = INT_LIT(0);
    }
    // Register attributer register for variable
    this->setLKO(v, reg);
    return reg;
}

Operand SSAIR::visitAssign(const parser::Assign& node) {
    const Operand rhs = this->visit(node.value());
    context::Symbol* s_lhs = this->m_ctx->getNodeSymbol(node.target());
    this->setLKO(s_lhs, rhs);
    return rhs;
}

Operand SSAIR::visitExprStmt(const parser::ExprStmt& node) {
    return this->visit(node.expr());
}

// The unit's value is its last statement's, so blast_main has something to
// return until real functions exist.
Operand SSAIR::visitTranslationUnit(const parser::TranslationUnit& node) {
    Operand last = NONE();
    for (const auto& stmt : node.stmts()) {
        last = this->visit(stmt.get());
    }
    return last;
}

Function SSAIR::run(const parser::TranslationUnit& unit) {
    Operand last = this->visit(&unit);
    if (last.m_kind == Operand::Kind::NONE) {
        last = INT_LIT(0);
    }
    this->addInstruction(last, NONE(), Opcode::RET);
    return this->currentFct();
}

} // namespace blast::core::ir
