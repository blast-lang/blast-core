#include "Parser.hpp"
#include <core/Exception.hpp>


namespace blast::core::parser {

void SimpleParser::parseStmt() {
    // 'x :: T' at statement level starts a declaration. The '::' only means
    // that here -- inside an expression it is an annotation -- which is why the
    // decision lives at this level and not in parserIdentifier().
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
}

void SimpleParser::parseDecl() {

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
}


void SimpleParser::parseExpr() {
    // Let's see the first token in the stip do decide what to parse
    switch (this->currentToken().m_kind) {
        // ---- Simple id
        case TokenKind::IDENDIFIER: {
            this->parserIdentifier();
            return;
        }
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
        default:
            throw ParseError("[Expr] Expected a literal or an identifier", m_pos);
    }
}

void SimpleParser::parseLiteral() {

}

// Pushes exactly one Identifier -- nothing else. What follows the identifier is
// the caller's business, since only the caller knows the context it is in.
void SimpleParser::parserIdentifier() {
    if (this->currentToken().m_kind != TokenKind::IDENDIFIER) {
        throw ParseError("[Identifier] Expected an identifier", m_pos);
    }
    const Token& tok = this->advance();
    this->push(std::make_unique<Identifier>(tok.m_value));
}

void SimpleParser::parseTranslationUnit() {
    auto unit = std::make_unique<TranslationUnit>();
    while (this->currentToken().m_kind != TokenKind::NONE) {
        this->parseStmt();
        unit->add(this->popAs<Stmt>());
    }
    this->push(std::move(unit));
}

void SimpleParser::buildAST() {
    this->parseTranslationUnit();
    this->m_root = this->popAs<TranslationUnit>();
}

} // namespace blast::core::parser