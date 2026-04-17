#pragma once
#include <core/Automata.hpp>
#include <stdexcept>
#include <string_view>

namespace blast::core::parser {

template<blast::core::Hashable T>
class StringAutomata : public blast::core::Automata<T> {
public:
    using blast::core::Automata<T>::Automata;
};

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
// Throws std::invalid_argument on syntax errors.
//  Grammar to support:
//  regex      ::= union
//  union      ::= concat ('|' concat)*
//  concat     ::= term+
//  term       ::= atom ('*' | '+' | '?')?
//  atom       ::= '(' regex ')' | literal
//  literal    ::= '\' char  |  any non-metachar
template<blast::core::Hashable T>
StringAutomata<T> parse_regex(std::basic_string_view<T> pattern);



} // namespace blast::core::parser
