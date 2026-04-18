#pragma once
#include <core/Automata.hpp>
#include <stdexcept>
#include <vector>

namespace blast::core {

template<std::totally_ordered T>
struct SymblRange {
    T lo, hi;
    SymblRange(T c) : lo(c), hi(c) {}
    SymblRange(T lo, T hi) : lo(lo), hi(hi) {}
    bool contains(const T& v) const { return this->lo <= v && v <= this->hi; }
    bool intersects(const SymblRange& s) const { return this->lo <= s.hi && s.lo <= this->hi; }
    auto operator<=>(const SymblRange&) const = default;
};

} // namespace blast::core

template<std::totally_ordered T>
    requires blast::core::Hashable<T>
struct std::hash<blast::core::SymblRange<T>> {
    size_t operator()(const blast::core::SymblRange<T>& r) const noexcept {
        std::hash<T> h;
        return h(r.lo) ^ (h(r.hi) << 1);
    }
};

namespace blast::core {

template<std::totally_ordered T>
    requires Hashable<T>
class RangeAutomata : public IAutomata {
    using StateInd = blast::core::StateInd;

protected:
    Automata<SymblRange<T>> m_dfa;
public:
    RangeAutomata(
        StateInd initial,
        std::set<StateInd> accepting,
        StateInd sink,
        std::vector<std::unordered_map<SymblRange<T>, StateInd>> transitions = {}
    ) : m_dfa(initial, std::move(accepting), sink, std::move(transitions)) {}

    RangeAutomata(const RangeAutomata&) = default;
    RangeAutomata(RangeAutomata&&) noexcept = default;

    RangeAutomata(const Automata<SymblRange<T>>& dfa): m_dfa(dfa) {}
    RangeAutomata(Automata<SymblRange<T>>&& dfa) noexcept: m_dfa(std::move(dfa)) {}
    RangeAutomata(const SymblRange<T>& s): m_dfa(s) {}

    template<std::ranges::input_range Range>
        requires std::same_as<std::ranges::range_value_t<Range>, T>
    explicit RangeAutomata(const Range& range) {
        std::vector<T> syms(std::ranges::begin(range), std::ranges::end(range));
        StateInd n = static_cast<StateInd>(syms.size());
        StateInd sink = n + 1;
        std::vector<std::unordered_map<SymblRange<T>, StateInd>> trans(n + 2);
        for (StateInd i = 0; i < n; ++i)
            trans[i][SymblRange<T>(syms[i])] = i + 1;
        this->m_dfa = Automata<SymblRange<T>>(0, {n}, sink, std::move(trans));
    }

    void reset() override { this->m_dfa.reset(); }
    bool accepts() const override { return this->m_dfa.accepts(); }

    const Automata<SymblRange<T>>& automata() const { return this->m_dfa; }

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
        for (const T& s : range)
            this->process(s);
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

} // namespace blast::core
