#pragma once
#include <core/RangeAutomata.hpp>
#include <string_view>
#include <utility>

namespace blast::core::parser {

template<typename T>
concept CharLike = std::same_as<T, char>     ||
                   std::same_as<T, wchar_t>  ||
                   std::same_as<T, char8_t>  ||
                   std::same_as<T, char16_t> ||
                   std::same_as<T, char32_t>;
                  
template<CharLike T>
using StringAutomata = blast::core::RangeAutomata<T>;

template<CharLike T>
// [a-z] | [A-Z]
inline const StringAutomata<T> letter = StringAutomata<T>((T)'a', (T)'z') || StringAutomata<T>((T)'A', (T)'Z');

template<CharLike T>
// [0-9]
inline const StringAutomata<T> digit = StringAutomata<T>((T)'0', (T)'9');

template<CharLike T>
inline const StringAutomata<T> alphanum = letter<T> || digit<T>;

template<CharLike T>
// [0-9]+
inline const StringAutomata<T> positive_natural = +digit<T>;

template<CharLike T>
inline const StringAutomata<T> underscore = StringAutomata<T>((T)'_');

template<CharLike T>
// [a-zA-Z_][a-zA-Z0-9_]*
inline const StringAutomata<T> identifier = (letter<T> || underscore<T>) + *(alphanum<T> || underscore<T>);


/*
template<CharLike T>
class Tokenizer: {
    
}
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

} // namespace blast::core::parser
