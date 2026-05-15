#include "Lexer.hpp"
#include <cctype>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace blast::core::lexer {


auto run_maximal_munch(StringAutomata<char>* dfa, std::string_view input, size_t pos) {
    dfa->reset();
    size_t last_accept = pos;
    while (pos < input.size()) {
        dfa->process(input[pos]);
        if (dfa->accepts()) {
            last_accept = pos + 1;
        } else if (dfa->stuck()) {
            break;
        }
        ++pos;
    }
    return last_accept;
}

std::vector<MetaLexer::Token> MetaLexer::process(std::string_view input) const {
    std::vector<MetaLexer::Token> tokens;
    size_t pos = 0;

    while (pos < input.size()) {
        size_t best_end = pos;
        MetaLexer::TokenKind best_kind = MetaLexer::TokenKind::NONE;
        for (auto& [dfa_ptr, kind]: MetaLexer::rules) {
            size_t end = run_maximal_munch(dfa_ptr, input, pos);
            if (end > best_end) { best_end = end; best_kind = kind; }
        }
        if (best_end == pos)
            throw std::runtime_error(std::string("Unknown character '") + input[pos] + "'");

        if (best_kind != MetaLexer::TokenKind::NONE) {
            tokens.push_back({best_kind, std::string(input.substr(pos, best_end - pos))});
        }
        pos = best_end;
    }
    return tokens;
}

} // namespace blast::core::lexer
