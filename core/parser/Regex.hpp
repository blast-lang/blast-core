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
class StringAutomata: public blast::core::RangeAutomata<T> {
    using RangeAutomata = blast::core::RangeAutomata<T>;
public:
    using RangeAutomata::RangeAutomata;
    StringAutomata(RangeAutomata&& r)       : RangeAutomata(std::move(r)) {}
    StringAutomata(const RangeAutomata& r)  : RangeAutomata(r) {}

    static const StringAutomata letter;      // [a-z] | [A-Z] | '_'
    static const StringAutomata digit;       // [0-9]
    static const StringAutomata number;      // [0-9]+
    static const StringAutomata identifier;  // [a-zA-Z_][a-zA-Z0-9_]*
};

template<CharLike T>
inline const StringAutomata<T> StringAutomata<T>::letter =
    StringAutomata<T>(blast::core::SymblRange<T>((T)'a', (T)'z')) ||
    StringAutomata<T>(blast::core::SymblRange<T>((T)'A', (T)'Z')) ||
    StringAutomata<T>(blast::core::SymblRange<T>((T)'_', (T)'_'));

template<CharLike T>
inline const StringAutomata<T> StringAutomata<T>::digit = StringAutomata<T>(blast::core::SymblRange<T>((T)'0', (T)'9'));

template<CharLike T>
inline const StringAutomata<T> StringAutomata<T>::number = +StringAutomata<T>::digit;

template<CharLike T>
inline const StringAutomata<T> StringAutomata<T>::identifier =
    StringAutomata<T>::letter + *(StringAutomata<T>::letter || StringAutomata<T>::digit);


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
