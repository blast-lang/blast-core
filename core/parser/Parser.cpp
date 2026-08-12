#include "Parser.hpp"
#include <core/Exception.hpp>
namespace blast::core::parser {

int binding_power(const SimpleParser::Token& t) {
    switch (t.m_kind) {
        case SimpleParser::TokenKind::ASSIGN: return 1;   // low, right-assoc
        case SimpleParser::TokenKind::BIN_OP:             // +,* differentiated by value
            if (t.m_value == "*" ) return 20;
            if (t.m_value == "+" ) return 10;
            return 0;
        default: return 0;
    }
}

/*


Why it works — walk through 1 + 2 * 3:

1. parse_expr(0) → lhs = 1. Sees + (bp 10) ≥ 0, so it consumes + and calls parse_expr(11) for the right side.
2. parse_expr(11) → lhs = 2. Sees * (bp 20) ≥ 11, consumes *, calls parse_expr(21).
3. parse_expr(21) → lhs = 3. Sees end (bp 0) < 21, returns 3.
4. Back in step 2: builds (2 * 3), loops, sees end < 11, returns (2 * 3).
5. Back in step 1: builds (1 + (2 * 3)). Correct — * bound tighter.

Now flip it to 1 * 2 + 3: when the inner parse_expr(21) (called after *) sees + with bp 10 < 21, it stops and returns just 2. So * only grabs 2, and + is left for the outer call — giving ((1 * 2) + 3). The min_bp floor is exactly what prevents the tighter operator from swallowing the looser one.

Associativity falls out of what floor you recurse with:
- bp + 1 (higher floor) → left-associative: a - b - c becomes (a - b) - c.
- bp (same floor) → right-associative: useful for = or ^ (a = b = c → a = (b = c)).

So min_bp is the single parameter that encodes both precedence and associativity, which is why one small recursive function replaces the usual ladder of parse_term / parse_factor / parse_unary functions.

The min_bp = 0 default just means "top-level call: accept operators of any precedence."



std::unique_ptr<Expr> SimpleParser::parse_expr(int min_bindpwr) {
    // 1. parse the left operand (a literal, identifier, par\-enthesized expr, ...)
    std::unique_ptr<Expr> lhs = parse_primary();

    // 2. keep folding in binary operators, as long as they bind tightly enough
    while (true) {
        int bp = binding_power(peek());   // 0 if the next token isn't a bin-op
        if (bp < min_bindpwr) break;       // <-- min_bindpwr stops the loop

        std::string op = advance().m_value;
        // right operand: recurse with a HIGHER floor so we only grab things
        // that bind tighter than the current operator (left-associativity)
        std::unique_ptr<Expr> rhs = parse_expr(bp + 1);

        lhs = std::make_unique<BinaryExpr>(op, std::move(lhs), std::move(rhs));
    }
    return lhs;
}
*/

std::unique_ptr<Stmt> SimpleParser::parse_stmt() {
    // Predictive lookahead.
    // Declaration:  IDENT '::' ...
    if (current_token().m_kind == TokenKind::IDENDIFIER &&
        next_token().m_kind    == TokenKind::COLON_COLON) {
        return this->parse_decl();
    }

    // Otherwise an expression statement:  EXPR ';'
    std::unique_ptr<Expr> expr = this->parse_expr(0);
    if (!expr) {
        return nullptr;
    }
    if (this->current_token().m_kind != TokenKind::ENDEXPR) {
        // TODO: Throw Expected ';'
        return nullptr;
    }
    this->advance();
    return std::make_unique<ExprStmt>(std::move(expr));
}

// Parse a leaf of the AST
std::unique_ptr<Expr> SimpleParser::parse_primary() {
    const Token& tok = this->current_token();
    switch (tok.m_kind) {
        case TokenKind::INT_LIT: {
            std::int64_t value = std::stoll(tok.m_value);
            this->advance();
            return std::make_unique<IntLiteral>(value);
        }
        case TokenKind::FLOAT_LIT: {
            double value = std::stod(tok.m_value);
            this->advance();
            return std::make_unique<FloatLiteral>(value);
        }
        case TokenKind::BOOL_LIT: {
            bool value = (tok.m_value == "true");
            this->advance();
            return std::make_unique<BoolLiteral>(value);
        }
        case TokenKind::STR_LIT: {
            // Strip the surrounding quotes the lexer kept.
            std::string value = tok.m_value;
            if (value.size() >= 2) {
                value = value.substr(1, value.size() - 2);
            }
            this->advance();
            return std::make_unique<StringLiteral>(std::move(value));
        }
        case TokenKind::IDENDIFIER: {
            std::string name = tok.m_value;
            this->advance();
            return std::make_unique<Identifier>(std::move(name));
        }
        default:
            // TODO: Throw Expected primary expression
            return nullptr;
    }
}

std::unique_ptr<Expr> SimpleParser::parse_expr(int min_bindpwr) {
    // `Pratt` / precedence climbing: parse a primary, then fold in infix operators
    // whose binding power is >= the current floor.
    std::unique_ptr<Expr> lhs = this->parse_primary();
    if (!lhs) {
        return nullptr;
    }

    while (true) {
        const Token& op_tok = this->current_token();
        int bp = binding_power(op_tok);
        if (bp == 0 || bp < min_bindpwr) {
            break;  // not an infix operator, or it binds too loosely to grab here
        }

        // Capture the operator, then consume it.
        const TokenKind op_kind = op_tok.m_kind;
        std::string op = op_tok.m_value;
        this->advance();

        // Assignment is right-associative (recurse at the same floor); the
        // arithmetic operators are left-associative (recurse one floor higher).
        const bool right_assoc = (op_kind == TokenKind::ASSIGN);
        std::unique_ptr<Expr> rhs = this->parse_expr(right_assoc ? bp : bp + 1);
        if (!rhs) {
            return nullptr;
        }

        if (op_kind == TokenKind::ASSIGN) {
            lhs = std::make_unique<Assign>(std::move(lhs), std::move(rhs));
        } else {
            lhs = std::make_unique<BinaryExpr>(std::move(op), std::move(lhs), std::move(rhs));
        }
    }
    return lhs;
}


std::unique_ptr<Decl> SimpleParser::parse_decl() {
    return this->parse_vardecl();
}

std::unique_ptr<VarDecl> SimpleParser::parse_vardecl() {
    // Read-only pointer to the current token. The tokenizer's token vector is
    // not mutated during parsing, so re-pointing after advance() stays valid --
    // this avoids copying the Token (and its std::string) at each step.
    const Token* tok = &this->current_token();
    std::string name, type;

    if (tok->m_kind == TokenKind::IDENDIFIER) {
        name = tok->m_value;
        this->advance();
        tok = &this->current_token();
    } else {
        // TODO: Throw Expected lvalue identifier
        return nullptr;
    }

    if (tok->m_kind == TokenKind::COLON_COLON) {
        this->advance();
        tok = &this->current_token();
    } else {
        // TODO: Throw
        return nullptr;
    }

    if (tok->m_kind == TokenKind::IDENDIFIER) {
        type = tok->m_value;
        this->advance();
        tok = &this->current_token();
    } else {
        // TODO: Throw Expected lvalue identifier
        return nullptr;
    }

    if (tok->m_kind == TokenKind::ENDEXPR) {
        this->advance();
    } else {
        // TODO: Throw
        return nullptr;
    }

    // Initialization part
    std::unique_ptr<Expr> init = nullptr;

    return std::make_unique<VarDecl>(
        std::move(name),
        std::make_unique<Identifier>(std::move(type)),
        std::move(init)
    );
}

std::unique_ptr<TranslationUnit> SimpleParser::parse_translation_unit() {
    auto unit = std::make_unique<TranslationUnit>();
    // Parse statements until NONE (end of the token stream).
    while (current_token().m_kind != TokenKind::NONE) {
        std::unique_ptr<Stmt> stmt = this->parse_stmt();
        if (!stmt) {
            // A parse failed without consuming; stop rather than spin forever.
            break;
        }
        unit->add(std::move(stmt));
    }
    return unit;
}

void SimpleParser::buildAST() {
    this->m_root = this->parse_translation_unit();
}

} // namespace blast::core::parser
