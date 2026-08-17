#include "Parser.hpp"
#include <core/Exception.hpp>


namespace blast::core::parser {

// Throws rather than defaulting to 0: a binding power of 0 would silently equal
// the empty-stack floor, so a new operator the lexer knows and the parser does
// not would misfold instead of being reported.
int bindingPower(const SimpleParser::Token& t) {
    switch (t.m_kind) {
        case SimpleParser::TokenKind::ASSIGN: return 1;   // low, right-assoc
        case SimpleParser::TokenKind::BIN_OP:             // +,* differentiated by value
            if (t.m_value == "*" ) return 20;
            if (t.m_value == "+" ) return 10;
            throw ParseError("[Expr] Unknown binary operator '" + t.m_value + "'", 0);
        default:
            throw ParseError("[Expr] Not an operator", 0);
    }
}

void SimpleParser::parseStmt() {
    if (this->currentToken().m_kind == TokenKind::IDENDIFIER &&
        this->nextToken().m_kind == TokenKind::COLON_COLON) {
        this->parseVarDecl();
    } else {
        this->parseExpr();
    }

    // Both branches leave an Expr on the stack (declarations are expressions,
    // as in Julia); at statement level its value is discarded.
    this->push(std::make_unique<ExprStmt>(this->popAs<Expr>()));

    // ';' terminates the statement.
    if (this->currentToken().m_kind != TokenKind::ENDEXPR) {
        throw ParseError("[Stmt] Expected ';' at the end of the statement", m_pos);
    }
    this->advance();
    return;
}

void SimpleParser::parseDecl() {
    return;
}

// 'name :: Type' with an optional '= init'.
void SimpleParser::parseVarDecl() {
    this->parserIdentifier();
    auto name = this->popAs<Identifier>();

    if (this->currentToken().m_kind != TokenKind::COLON_COLON) {
        throw ParseError("[VarDecl] Expected '::' after the declared name", m_pos);
    }
    this->advance();

    // Type annotation: an identifier for now, a type expression later.
    this->parserIdentifier();
    auto type = this->popAs<Identifier>();

    // Initializer is optional: 'a::Int;' declares without initialising.
    std::unique_ptr<Expr> init = nullptr;
    if (this->currentToken().m_kind == TokenKind::ASSIGN) {
        this->advance();
        this->parseExpr();
        init = this->popAs<Expr>();
    }

    this->push(std::make_unique<VarDecl>(name->name(), std::move(type), std::move(init)));
    return;
}


// Shunting-yard: operands go straight to the parse stack, operators wait on
// m_operator_stack until one that binds less tightly arrives and forces a fold.
void SimpleParser::parseExpr() {
    // Operators already on the stack belong to an enclosing expression -- a
    // parenthesised group must never fold past the ones its caller left behind.
    const std::size_t base = this->operatorDepth();

    while (true) {
        this->parseOperand();
        const Token op = this->currentToken();
        if (!(op.m_kind == TokenKind::BIN_OP || op.m_kind == TokenKind::ASSIGN)) {
            break;
        }

        // Left-assoc: an operator of equal precedence already on the stack folds first
        // so 8 + 9 + 10 -> (8 + 9) + 10. Right-assoc: only a strictly
        // tighter one folds, so a = b = c -> a = (b = c).
        const int bp = bindingPower(op);
        const bool right_assoc = (op.m_kind == TokenKind::ASSIGN);
        while (this->operatorDepth() > base &&
               (right_assoc ? bindingPower(this->topOperator()) >  bp
                            : bindingPower(this->topOperator()) >= bp)
        ) {
            this->parseBinaryExpr();
        }
        this->pushOperator(op);
        this->advance();
    }

    // End of the expression: fold what is left, down to our own floor.
    while (this->operatorDepth() > base) {
        this->parseBinaryExpr();
    }
    return;
}

// Exactly one operand, or the expression is malformed: parseBinaryExpr pops two
// of these, so failing to push one here would underflow the parse stack.
void SimpleParser::parseOperand() {
    switch (this->currentToken().m_kind) {
        case TokenKind::IDENDIFIER:
            this->parserIdentifier();
            return;
        case TokenKind::INT_LIT:
        case TokenKind::FLOAT_LIT:
        case TokenKind::BOOL_LIT:
        case TokenKind::STR_LIT:
            this->parseLiteral();
            return;
        case TokenKind::OPEN_PAR: {
            this->advance();
            // Fresh floor: the group process everything up to its ')'.
            this->parseExpr();
            if (this->currentToken().m_kind != TokenKind::CLOSE_PAR) {
                throw ParseError("[Expr] No closing parenthesis", m_pos);
            }
            this->advance();
            return;
        }
        default:
            throw ParseError("[Expr] Expected an operand", m_pos);
    }
}

void SimpleParser::parseUnaryExpr() {

}

// Folds the operator on top of m_operator_stack with the two operands on top of
// the parse stack. Purely a stack operation -- it reads no tokens, so the caller
// that decided to fold now is the one that decided precedence.
void SimpleParser::parseBinaryExpr() {
    Token op  = this->popOperator();
    // rhs comes off first as it was pushed last
    auto  rhs = this->popAs<Expr>();
    auto  lhs = this->popAs<Expr>();

    if (op.m_kind == TokenKind::ASSIGN) {
        this->push(std::make_unique<Assign>(std::move(lhs), std::move(rhs)));
    } else {
        this->push(std::make_unique<BinaryExpr>(
            std::move(op.m_value),
            std::move(lhs),
            std::move(rhs)
        ));
    }
    return;
}

void SimpleParser::parseLiteral() {
    // Let's see the first token in the stip do decide what to parse
    switch (this->currentToken().m_kind) {
        //  ---- Literals
        case TokenKind::INT_LIT: {
            std::int64_t value = std::stoll(this->currentToken().m_value);
            this->advance();
            this->push(std::make_unique<IntLiteral>(value));
            return;
        }
        case TokenKind::FLOAT_LIT: {
            double value = std::stod(this->currentToken().m_value);
            this->advance();
            this->push(std::make_unique<FloatLiteral>(value));
            return;
        }
        case TokenKind::BOOL_LIT: {
            bool value = (this->currentToken().m_value == "true");
            this->advance();
            this->push(std::make_unique<BoolLiteral>(value));
            return;
        }
        case TokenKind::STR_LIT: {
            // Strip the surrounding quotes the lexer kept.
            std::string value = this->currentToken().m_value;
            if (value.size() >= 2) {
                value = value.substr(1, value.size() - 2);
            }
            this->advance();
            this->push(std::make_unique<StringLiteral>(std::move(value)));
            return;
        }
    }
    return;
}

// Pushes exactly one Identifier -- nothing else. What follows the identifier is
// the caller's business, since only the caller knows the context it is in.
void SimpleParser::parserIdentifier() {
    if (this->currentToken().m_kind != TokenKind::IDENDIFIER) {
        throw ParseError("[Identifier] Expected an identifier", m_pos);
    }
    const Token& tok = this->advance();
    this->push(std::make_unique<Identifier>(tok.m_value));
    return;
}

void SimpleParser::parseTranslationUnit() {
    auto unit = std::make_unique<TranslationUnit>();
    while (this->currentToken().m_kind != TokenKind::NONE) {
        this->parseStmt();
        unit->add(this->popAs<Stmt>());
    }
    this->push(std::move(unit));
    return;
}

void SimpleParser::buildAST() {
    this->parseTranslationUnit();
    this->m_root = this->popAs<TranslationUnit>();
    return;
}

} // namespace blast::core::parser