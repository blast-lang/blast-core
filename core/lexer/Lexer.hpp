#pragma once
#include <core/RangeAutomata.hpp>
#include <string>
#include <string_view>
#include <vector>
#include <tuple>
#include <limits>

namespace blast::core::lexer {

void lex();

template<typename T>
concept CharLike = std::same_as<T, char>     ||
                   std::same_as<T, wchar_t>  ||
                   std::same_as<T, char8_t>  ||
                   std::same_as<T, char16_t> ||
                   std::same_as<T, char32_t>;

template<CharLike T>
using StringAutomata = blast::core::RangeAutomata<T>;



class MetaLexer {
public:
    using SA = StringAutomata<char>;
    enum class TokenKind {
        NONE,
        IDENTIFIER,
        END_RULE,
        LEXEM,
        // TODO: Range [a-A]
        RANGE,
        // TODO: Capture field <...>
        CAPTURE,
        BIN_OP,
        UNA_OP
    };

    struct Token {
        TokenKind m_kind;
        std::string m_value;
    };



    // Utils
    inline static SA letter             = SA('a', 'z') || SA('A', 'Z');
    inline static SA non_quote          = SA(std::numeric_limits<char>::min(), (char)('\''-1)) || SA((char)('\''+1), std::numeric_limits<char>::max());
    inline static SA end_rule           = SA(';');
    // Characters ignored in the grammar
    inline static SA newline            = SA('\n');
    inline static SA whitespace         = SA(' ') || SA('\t');

    // Grammar operators
    inline static SA identifier         = +letter;
    // Binaries
    inline static SA rule_concat        = SA('+');
    inline static SA rule_union         = SA('|') + SA('|');
    inline static SA capture            = SA('<') + identifier + SA('>');
    // Unaries
    inline static SA rule_start         = SA('*');
    inline static SA rule_plus          = SA('+');
    inline static SA rule_affect        = SA('=');
    // Language constants
    inline static SA lexem              = SA('\'') + (+non_quote) + SA('\'');

    inline static SA range              = SA('[') + (SA('\'') + non_quote + SA('\'')) + SA('-') + (SA('\'') + non_quote + SA('\'')) + SA(']');
    
    // Grammar rules
    inline static SA rule_definition    = identifier + rule_affect + identifier + end_rule;
public:

    // Rules for differenciating tokens
    inline static std::pair<SA*, TokenKind> rules[] = {
        {&MetaLexer::whitespace,    MetaLexer::TokenKind::NONE},
        {&MetaLexer::newline,       MetaLexer::TokenKind::NONE},
        {&MetaLexer::identifier,    MetaLexer::TokenKind::IDENTIFIER},
        {&MetaLexer::end_rule,      MetaLexer::TokenKind::END_RULE},
        {&MetaLexer::rule_concat,   MetaLexer::TokenKind::BIN_OP},
        {&MetaLexer::rule_union,    MetaLexer::TokenKind::BIN_OP},
        {&MetaLexer::rule_affect,   MetaLexer::TokenKind::BIN_OP},
        {&MetaLexer::capture,       MetaLexer::TokenKind::CAPTURE},
        {&MetaLexer::range,         MetaLexer::TokenKind::RANGE},
        {&MetaLexer::rule_start,    MetaLexer::TokenKind::UNA_OP},
        {&MetaLexer::rule_plus,     MetaLexer::TokenKind::UNA_OP},
        {&MetaLexer::lexem,         MetaLexer::TokenKind::LEXEM},
    };

    std::vector<MetaLexer::Token> process(std::string_view input) const;
};

class Tokenizer {
public:
    enum class TokenKind {
        // Utils
        NONE,
        // Literals
        INT_LIT,
        FLOAT_LIT,
        BOOL_LIT,
        STR_LIT,
        // Keywords
        UNDERSCORE,
        COMMA,
        ENDEXPR,
        OPEN_PAR,
        CLOSE_PAR,
        OPEN_BRAC,
        CLOSE_BRAC,
        OPEN_CURLBRAC,
        CLOSE_CURLBRAC,
        IF_STMT,
        ELSE_STMT,
        CONTINUE_STMT,
        // Operators
        BIN_OP,
        ASSIGN,
        UNA_OP,
        COLON_COLON,
        // Identifier
        IDENDIFIER,
    };

    struct Token {
        Tokenizer::TokenKind m_kind;
        std::string m_value;
    };

public:
    using SA = StringAutomata<char>;

public:
    Tokenizer();
    const std::vector<Tokenizer::Token>& tokens() const;

    // Human-readable name for a token kind (diagnostics, dumps).
    static std::string_view kindName(Tokenizer::TokenKind kind);

    // Processing
    void process(std::string_view input);

protected:
    // Set of rules
    std::vector<std::tuple<SA, size_t, Tokenizer::TokenKind>> m_rules;
    // Output stack
    std::vector<Tokenizer::Token> m_tokens;
};




} // namespace blast::core::lexer
