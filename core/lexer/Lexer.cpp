#include "Lexer.hpp"
#include <core/Exception.hpp>
#include <cctype>
#include <cstdio>
#include <string>
#include <algorithm>
#include <iostream>

namespace blast::core::lexer {

Tokenizer::Tokenizer(): m_rules(), m_tokens() {
    // Utils
    Tokenizer::SA letter             = SA('a', 'z') || SA('A', 'Z');
    Tokenizer::SA digit              = SA('0', '9');
    Tokenizer::SA alphanum           = letter || digit;
    // characters ignored in the grammar
    Tokenizer::SA newline            = SA('\n') || SA('\r');
    Tokenizer::SA whitespace         = SA(' ') || SA('\t');
    // Literals
    Tokenizer::SA integer_literal    = +digit;
    Tokenizer::SA float_literal      = +digit + SA('.') + +digit;
    Tokenizer::SA bool_literal       = SA(std::string_view{"true"}) || SA(std::string_view{"false"});
    Tokenizer::SA non_quote          = SA(std::numeric_limits<char>::min(), (char)('"'-1)) || SA((char)('"'+1), std::numeric_limits<char>::max());
    Tokenizer::SA string_literal     = SA('"') + *non_quote + SA('"');
    // Keywords
    Tokenizer::SA underscore         = SA('_');
    Tokenizer::SA comma              = SA(',');
    Tokenizer::SA endexpr            = SA(';');
    Tokenizer::SA open_par           = SA('(');
    Tokenizer::SA close_par          = SA(')');
    Tokenizer::SA open_brac          = SA('[');
    Tokenizer::SA close_brac         = SA(']');
    Tokenizer::SA open_curlbrac      = SA('{');
    Tokenizer::SA close_curlbrac     = SA('}');
    Tokenizer::SA continue_stmt      = SA(std::string_view{"continue"});
    Tokenizer::SA if_stmt            = SA(std::string_view{"if"});
    Tokenizer::SA else_stmt          = SA(std::string_view{"else"});
    // Operators
    Tokenizer::SA op_add             = SA('+');
    Tokenizer::SA op_mul             = SA('*');
    // Identifier: [a-zA-Z_][a-zA-Z0-9_]*
    Tokenizer::SA identifier         = (letter || underscore) + *(alphanum || underscore);

    // Register rules (dfa -> priority -> output token)
    this->m_rules.push_back({whitespace,        1000,  Tokenizer::TokenKind::NONE});
    this->m_rules.push_back({newline,           1000,  Tokenizer::TokenKind::NONE});
    // Literals
    this->m_rules.push_back({integer_literal,   0,      Tokenizer::TokenKind::INT_LIT});
    this->m_rules.push_back({float_literal,     0,      Tokenizer::TokenKind::FLOAT_LIT});
    this->m_rules.push_back({bool_literal,      0,      Tokenizer::TokenKind::BOOL_LIT});
    this->m_rules.push_back({string_literal,    0,      Tokenizer::TokenKind::STR_LIT});
    // Keywords
    this->m_rules.push_back({underscore,        0,      Tokenizer::TokenKind::UNDERSCORE});
    this->m_rules.push_back({comma,             0,      Tokenizer::TokenKind::COMMA});
    this->m_rules.push_back({endexpr,           0,      Tokenizer::TokenKind::ENDEXPR});
    this->m_rules.push_back({open_par,          0,      Tokenizer::TokenKind::OPEN_PAR});
    this->m_rules.push_back({close_par,         0,      Tokenizer::TokenKind::CLOSE_PAR});
    this->m_rules.push_back({open_brac,         0,      Tokenizer::TokenKind::OPEN_BRAC});
    this->m_rules.push_back({close_brac,        0,      Tokenizer::TokenKind::CLOSE_BRAC});
    this->m_rules.push_back({open_curlbrac,     0,      Tokenizer::TokenKind::OPEN_CURLBRAC});
    this->m_rules.push_back({close_curlbrac,    0,      Tokenizer::TokenKind::CLOSE_CURLBRAC});
    
    this->m_rules.push_back({continue_stmt,     0,      Tokenizer::TokenKind::CONTINUE_STMT});
    this->m_rules.push_back({if_stmt,           0,      Tokenizer::TokenKind::IF_STMT});
    this->m_rules.push_back({else_stmt,         0,      Tokenizer::TokenKind::ELSE_STMT});
    // Operators
    this->m_rules.push_back({op_add,            0,      Tokenizer::TokenKind::BIN_OP});
    this->m_rules.push_back({op_mul,            0,      Tokenizer::TokenKind::BIN_OP});
    // Identifier: [a-zA-Z_][a-zA-Z0-9_]*
    this->m_rules.push_back({identifier,        1,      Tokenizer::TokenKind::IDENDIFIER});

    // Sort rules by priority
    std::sort(this->m_rules.begin(), this->m_rules.end(),
        [](const std::tuple<SA, size_t, TokenKind>& a, const std::tuple<SA, size_t, TokenKind>& b) {
            return std::get<1>(a) > std::get<1>(b);
        }
    );
}

const std::vector<Tokenizer::Token>& Tokenizer::tokens() const {
    return this->m_tokens;
}

void Tokenizer::print() const {

    auto tts = [](Tokenizer::TokenKind tk) {
        switch (tk) {
        case Tokenizer::TokenKind::NONE: return "NONE";
        case Tokenizer::TokenKind::INT_LIT: return "INT_LIT";
        case Tokenizer::TokenKind::FLOAT_LIT: return "FLOAT_LIT";
        case Tokenizer::TokenKind::BOOL_LIT: return "BOOL_LIT";
        case Tokenizer::TokenKind::STR_LIT: return "STR_LIT";
        case Tokenizer::TokenKind::UNDERSCORE: return "UNDERSCORE";
        case Tokenizer::TokenKind::COMMA: return "COMMA";
        case Tokenizer::TokenKind::ENDEXPR: return "ENDEXPR";
        case Tokenizer::TokenKind::OPEN_PAR: return "OPEN_PAR";
        case Tokenizer::TokenKind::CLOSE_PAR: return "CLOSE_PAR";
        case Tokenizer::TokenKind::OPEN_BRAC: return "OPEN_BRAC";
        case Tokenizer::TokenKind::CLOSE_BRAC: return "CLOSE_BRAC";
        case Tokenizer::TokenKind::OPEN_CURLBRAC: return "OPEN_CURLBRAC";
        case Tokenizer::TokenKind::CLOSE_CURLBRAC: return "CLOSE_CURLBRAC";
        case Tokenizer::TokenKind::IF_STMT: return "IF_STMT";
        case Tokenizer::TokenKind::ELSE_STMT: return "ELSE_STMT";
        case Tokenizer::TokenKind::CONTINUE_STMT: return "CONTINUE_STMT";
        case Tokenizer::TokenKind::BIN_OP: return "BIN_OP";
        case Tokenizer::TokenKind::UNA_OP: return "UNA_OP";
        case Tokenizer::TokenKind::IDENDIFIER: return "IDENDIFIER";
            default: return "UNKNOWN";
        }
    };
    for (const Tokenizer::Token& token: this->m_tokens) {
        //std::cout << "[" << tts(token.m_kind) << "]" << "\t\t\t" << token.m_value << "\n";
        std::printf("[%s]   \t%s\n", tts(token.m_kind), token.m_value.c_str());
    }
}

size_t run_maximal_munch(Tokenizer::SA& rule, std::string_view input, size_t pos) {
    rule.reset();
    size_t last_accept = pos;
    while (pos < input.size()) {
        rule.process(input[pos]);
        if (rule.accepts()) {
            last_accept = pos + 1;
        } else if (rule.stuck()) {
            break;
        }
        ++pos;
    }
    return last_accept;
}

void Tokenizer::process(std::string_view input) {
    size_t pos = 0;
    size_t end = 0;
    size_t best_end = 0;
    size_t best_rule = 0;
    Tokenizer::TokenKind best_kind = Tokenizer::TokenKind::NONE;

    while (pos < input.size()) {
        best_end = end = pos;
        best_rule = 0;
        best_kind = Tokenizer::TokenKind::NONE;
        // Let's find the rule that accepts the longest stream (maximum munch)
        for (size_t i(0); i < this->m_rules.size(); i++) {
            Tokenizer::SA& rule = std::get<0>(this->m_rules[i]);
            end = run_maximal_munch(rule, input, pos);
            if (end > best_end) {
                best_end = end;
                best_rule = i;
            } else if (end == best_end && end > pos) {
                // Same length: lower priority rule wins
                if (std::get<1>(this->m_rules[i]) < std::get<1>(this->m_rules[best_rule])) {
                    best_rule = i;
                }
            }
        }
        if (best_end == pos) {
            throw blast::core::LexError(std::string("Unknown input '") + std::string(1, input[pos]) + "' (char=" + std::to_string((int)(unsigned char)input[pos]) + ")", pos);
        }
        if (std::get<2>(this->m_rules[best_rule]) != Tokenizer::TokenKind::NONE) {
            this->m_tokens.push_back({std::get<2>(this->m_rules[best_rule]), std::string(input.substr(pos, best_end - pos))});
        }
        pos = best_end;
    }
}


} // namespace blast::core::lexer
