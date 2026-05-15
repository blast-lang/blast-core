#pragma once
#include <core/RangeAutomata.hpp>
#include <string>
#include <string_view>
#include <vector>
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


/*
enum class TokenKind {
    NONE,
    IDENDIFIER,
    INT_LIT,
    FLOAT_LIT,
    BOOL_LIT,
    STR_LIT,
    IF_STMT,
    ELSE_STMT,
};



class Tokenizer {
public:
    using SA = StringAutomata<char>;

    // Utils
    inline static SA letter             = SA('a', 'z') || SA('A', 'Z');
    inline static SA digit              = SA('0', '9');
    inline static SA alphanum           = letter || digit;
    inline static SA underscore         = SA('_');
    // Literals
    inline static SA integer_literal    = +digit;
    inline static SA float_literal      = +digit + SA('.') + +digit;
    inline static SA bool_literal       = SA(std::string_view{"true"}) || SA(std::string_view{"false"});
    inline static SA non_quote          = SA((char)0, (char)('"'-1)) || SA((char)('"'+1), std::numeric_limits<char>::max());
    inline static SA string_literal     = SA('"') + *non_quote + SA('"');

    // Identifier: [a-zA-Z_][a-zA-Z0-9_]*
    inline static SA identifier         = (letter || underscore) + *(alphanum || underscore);

    // characters ignored in the grammar
    inline static SA newline            = SA('\n');
    inline static SA whitespace         = SA(' ') || SA('\t');

    // Starting point
    inline static SA main               = *whitespace + identifier + *whitespace;


    // Rules for differenciating tokens
    inline static std::pair<SA*, TokenKind> rules[] = {
        {&Tokenizer::whitespace, TokenKind::NONE},
        {&Tokenizer::newline,    TokenKind::NONE},
        {&Tokenizer::integer_literal, TokenKind::INT_LIT},
        {&Tokenizer::float_literal, TokenKind::FLOAT_LIT},
        {&Tokenizer::bool_literal, TokenKind::BOOL_LIT},
        {&Tokenizer::string_literal, TokenKind::STR_LIT},
        {&Tokenizer::identifier, TokenKind::IDENDIFIER}
    };

    std::vector<Token> process(std::string_view input) const;
};
*/


// Parses a regex pattern and returns a DFA recognizing its language.
//
// Supported syntax:
//   a       literal character
//   ab      concatenation
//   a|b     union
//   a*      zero or more (Kleene star)
//   a+      one or more (Kleene plus)
//   a?      zero or one (optional)
//   (...)   grouping
//   \x      escaped metacharacter
//
// Grammar:
//   regex      ::= union
//   union      ::= concat ('|' concat)*
//   concat     ::= term+
//   term       ::= atom ('*' | '+' | '?')?
//   atom       ::= '(' regex ')' | literal
//   literal    ::= '\' char  |  any non-metachar
//
// Throws std::invalid_argument on syntax errors.
//template<blast::core::Hashable T>
//StringAutomata<T> parse_regex(std::basic_string_view<T> pattern);

} // namespace blast::core::lexer
