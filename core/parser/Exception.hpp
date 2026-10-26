#pragma once
#include <core/Exception.hpp>
#include <core/lexer/Lexer.hpp>

namespace blast::core::parser {

namespace detail {

inline std::string describeToken(const lexer::Tokenizer::Token& token) {
    if (token.m_kind == lexer::Tokenizer::TokenKind::NONE) {
        return "end of input";
    }
    return std::string(lexer::Tokenizer::kindName(token.m_kind)) + " '" + token.m_value + "'";
}

inline std::string describeKinds(const std::vector<lexer::Tokenizer::TokenKind>& kinds) {
    std::string out;
    for (std::size_t i(0); i < kinds.size(); i++) {
        out += (i == 0 ? "" : " or ") + std::string(lexer::Tokenizer::kindName(kinds[i]));
    }
    return out;
}

} // namespace detail

class UnexpectedToken : public ParseError {
public:
    using Token = lexer::Tokenizer::Token;
    using TokenKind = lexer::Tokenizer::TokenKind;

    UnexpectedToken(std::vector<TokenKind> expected, Token found, std::size_t position):
        ParseError("Expected " + detail::describeKinds(expected) + " but found " + detail::describeToken(found), position, found.m_location),
        m_expected(std::move(expected)), m_found(std::move(found)) {}

    const std::vector<TokenKind>& expected() const { return m_expected; }
    const Token& found() const { return m_found; }

private:
    std::vector<TokenKind> m_expected;
    Token m_found;
};

class UnknownOperator : public ParseError {
public:
    using Token = lexer::Tokenizer::Token;

    UnknownOperator(Token op, std::size_t position):
        ParseError("Unknown binary operator '" + op.m_value + "'", position, op.m_location),
        m_op(std::move(op)) {}

    const Token& op() const { return m_op; }

private:
    Token m_op;
};

} // namespace blast::core::parser
