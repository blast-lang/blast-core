#pragma once
#include <stack>
#include <core/parser/Ast.hpp>
#include <core/lexer/Lexer.hpp>

namespace blast::core::parser {

class SimpleParser {
public:
    using Tokenizer = blast::core::lexer::Tokenizer;
    using Token     = Tokenizer::Token;
    using TokenKind = Tokenizer::TokenKind;

    explicit SimpleParser(const Tokenizer& tokenizer): m_tokenizer(tokenizer), m_pos(0) {}

// Utils
private:
    // Token at the cursor
    const Token& current_token() const { return token_at(m_pos); }
    // One-token lookahead
    const Token& next_token() const { return token_at(m_pos + 1); }

    // Consume the current token and move to the next, returning the one consumed.
    // Stops advancing past the end so current_token() stays on the NONE sentinel.
    const Token& advance() {
        const Token& consumed = current_token();
        if (m_pos < m_tokenizer.tokens().size()) {
            ++m_pos;
        }
        return consumed;
    }

    const Token& token_at(std::size_t pos) const {
        static const Token eof{TokenKind::NONE, ""};
        const auto& tokens = m_tokenizer.tokens();
        return pos < tokens.size() ? tokens[pos] : eof;
    }

// Parsing
protected:
    std::unique_ptr<TranslationUnit> parse_translation_unit();
    std::unique_ptr<Stmt>            parse_stmt();
    std::unique_ptr<Decl>            parse_decl();
    std::unique_ptr<VarDecl>         parse_vardecl();

    // Minimum binding power for precedent climbing
    std::unique_ptr<Expr>            parse_expr(int min_bindpwr = 0);
    // A leaf/primary expression: literal or identifier.
    std::unique_ptr<Expr>            parse_primary();

protected:
    const Tokenizer& m_tokenizer;
    // Cursor into m_tokenizer.tokens(); the read position lives in the parser,
    // not the tokenizer, so the token stream stays reusable.
    std::size_t m_pos;
    // Root of the AST, owned by the parser and populated by buildAST().
    std::unique_ptr<TranslationUnit> m_root;
public:
    void buildAST();
    // Root of the parsed AST (null until buildAST() runs).
    const TranslationUnit* root() const { return m_root.get(); }
};

} // namespace blast::core::parser