#pragma once
#include <core/Automata.hpp>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <utility>

namespace blast::core::parser {

// A symbol range [lo, hi]: matches any T where lo <= v <= hi.
// range(c) is a single-character range equivalent to range(c, c).
template<std::totally_ordered T>
struct SymblRange {
    T lo, hi;
    SymblRange(T c) : lo(c), hi(c) {}
    SymblRange(T lo, T hi) : lo(lo), hi(hi) {}
    bool contains(const T& v) const { return this->lo <= v && v <= this->hi; }
    bool intersects(const SymblRange& s) const { return this->lo <= s.hi && s.lo <= this->hi; }
    // default on <=> generates all six operators (<, <=, >, >=, ==, !=) with lexicographic comparison on {lo, hi}
    auto operator<=>(const SymblRange&) const = default;
};

} // namespace blast::core::parser

template<std::totally_ordered T>
    requires blast::core::Hashable<T>
struct std::hash<blast::core::parser::SymblRange<T>> {
    size_t operator()(const blast::core::parser::SymblRange<T>& r) const noexcept {
        std::hash<T> h;
        return h(r.lo) ^ (h(r.hi) << 1);
    }
};

namespace blast::core::parser {

template<std::totally_ordered T>
    requires blast::core::Hashable<T>
class RangeAutomata : public blast::core::IAutomata {
    using StateInd = blast::core::StateInd;

protected:
    blast::core::Automata<SymblRange<T>> m_dfa;
public:
    RangeAutomata(
        StateInd initial,
        std::set<StateInd> accepting,
        StateInd sink,
        std::vector<std::unordered_map<SymblRange<T>, StateInd>> transitions = {}
    ) : m_dfa(initial, std::move(accepting), sink, std::move(transitions)) {}

    RangeAutomata(const RangeAutomata&) = default;
    RangeAutomata(RangeAutomata&&) noexcept = default;

    RangeAutomata(const blast::core::Automata<SymblRange<T>>& dfa): m_dfa(dfa) {}
    RangeAutomata(blast::core::Automata<SymblRange<T>>&& dfa) noexcept: m_dfa(std::move(dfa)) {}

    // Build a chain automata recognizing exactly the sequence of symbols in `range`
    template<std::ranges::input_range Range>
        requires std::same_as<std::ranges::range_value_t<Range>, T>
    explicit RangeAutomata(const Range& range) {
        std::vector<T> syms(std::ranges::begin(range), std::ranges::end(range));
        StateInd n = static_cast<StateInd>(syms.size());
        StateInd sink = n + 1;
        std::vector<std::unordered_map<SymblRange<T>, StateInd>> trans(n + 2);
        for (StateInd i = 0; i < n; ++i) {
            trans[i][SymblRange<T>(syms[i])] = i + 1;
        }
        this->m_dfa = blast::core::Automata<SymblRange<T>>(0, {n}, sink, std::move(trans));
    }

    void reset() override { this->m_dfa.reset(); }
    bool accepts() const override { return this->m_dfa.accepts(); }

    // Expose inner DFA for NFA construction in operators
    const blast::core::Automata<SymblRange<T>>& automata() const { return this->m_dfa; }

    // Process the first range transition that contains `s`
    void process(const T& s) {
        for (auto const& [range, next] : this->m_dfa.out_transitions(this->m_dfa.current())) {
            if (range.contains(s)) {
                this->m_dfa.force_state(next);
                return;
            }
        }
        this->m_dfa.force_state(this->m_dfa.sink());
    }

    template<std::ranges::input_range Range>
        requires std::same_as<std::ranges::range_value_t<Range>, T>
    void process(const Range& range) {
        for (const T& s : range) {
            this->process(s);
        }
    }

    friend RangeAutomata operator||(const RangeAutomata& a, const RangeAutomata& b) {
        return RangeAutomata(a.m_dfa || b.m_dfa);
    }

    friend RangeAutomata operator+(const RangeAutomata& a, const RangeAutomata& b) {
        return RangeAutomata(a.m_dfa + b.m_dfa);
    }

    friend RangeAutomata operator+(const RangeAutomata& a) {
        return RangeAutomata(+a.m_dfa);
    }

    friend RangeAutomata operator*(const RangeAutomata& a) {
        return RangeAutomata(*a.m_dfa);
    }

    friend RangeAutomata operator~(const RangeAutomata& a) {
        return RangeAutomata(~a.m_dfa);
    }
};

template<typename T>
concept CharLike = std::same_as<T, char>     ||
                   std::same_as<T, wchar_t>  ||
                   std::same_as<T, char8_t>  ||
                   std::same_as<T, char16_t> ||
                   std::same_as<T, char32_t>;

template<CharLike T>
using StringAutomata = RangeAutomata<T>;


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
