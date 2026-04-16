#pragma once
#include <cstdint>
#include <concepts>
#include <functional>
#include <set>
#include <unordered_map>
#include <vector>
#include <optional>
#include <ranges>

namespace blast::core {

template<typename T>
concept Hashable = requires(T v) {
    { std::hash<T>{}(v) } -> std::convertible_to<std::size_t>;
};

using StateInd = size_t;
class IAutomata {
public:
    virtual ~IAutomata() = default;
    virtual void reset() = 0;
    virtual bool accepts() const = 0;
};

//  ε-edge

// Non-Deterministic Finite State Automata (NFA)
// Used as a construction intermediate only — convert to DFA via to_dfa()
template<Hashable T>
class NFA {
public:
    NFA(
        std::set<StateInd> initials, std::set<StateInd> acceptings,
        StateInd sink,
        std::vector<std::unordered_map<T, StateInd>> transitions = {},
        std::vector<std::set<StateInd>> epsilon = {}
    ): m_initials(std::move(initials)),
        m_acceptings(std::move(acceptings)), m_sink(sink),
        m_transitions(std::move(transitions)),
        m_epsilon(std::move(epsilon))
    {}

    NFA(const NFA& other):
        m_initials(other.m_initials),
        m_acceptings(other.m_acceptings), m_sink(other.m_sink),
        m_transitions(other.m_transitions),
        m_epsilon(other.m_epsilon)
    {}

    NFA(NFA&& other) noexcept:
        m_initials(std::move(other.m_initials)),
        m_acceptings(std::move(other.m_acceptings)), m_sink(other.m_sink),
        m_transitions(std::move(other.m_transitions)),
        m_epsilon(std::move(other.m_epsilon))
    {}

    // Build the simple transition q0 --symbol--> q1 (accept)
    NFA(const T& s):
        m_initials({0}),
        m_acceptings({1}), m_sink(2),
        m_transitions({
            {{s, 1}},  // q0 --s--> q1 (accept)
            {},        // q1 accept
            {}         // q2 sink
        }),
        m_epsilon(3)   // one empty ε-set per state
    {}

    // Build a chain q0 --s[0]--> q1 ... qn (accept), qn+1 (sink)
    template<std::ranges::input_range Range>
        requires std::same_as<std::ranges::range_value_t<Range>, T>
    NFA(const Range& range) {
        StateInd state = 0;
        for (const T& s : range) {
            this->m_transitions.push_back({{s, state + 1}});
            this->m_epsilon.push_back({});
            ++state;
        }
        this->m_transitions.push_back({});  // qn accept
        this->m_epsilon.push_back({});
        this->m_transitions.push_back({});  // qn+1 sink
        this->m_epsilon.push_back({});
        this->m_initials   = {0};
        this->m_acceptings = {state};
        this->m_sink       = state + 1;
    }

protected:
    // Index of the initial states
    std::set<StateInd> m_initials;
    // Set of accepting states
    std::set<StateInd> m_acceptings;
    // Index of the unique sink/trash state
    StateInd m_sink;
    // Transition table for each state: |m_transitions| = |m_states|
    std::vector<std::unordered_map<T, StateInd>> m_transitions;
    // m_epsilon[i] = ε-reachable states from i
    std::vector<std::set<StateInd>> m_epsilon;
};

// Deterministic finite state automata with fixed number of states
// https://en.wikipedia.org/wiki/Deterministic_finite_automaton
// https://cs.wellesley.edu/~cs235/fall10/lectures/14_DFA_operations_revised_2.pdf
template<Hashable T>
class Automata : public IAutomata {
public:
    Automata(
        StateInd initial, std::set<StateInd> accepting,
        StateInd sink,
        std::vector<std::unordered_map<T, StateInd>> transitions = {}
    ): m_current(initial), m_initial(initial),
        m_accepting(std::move(accepting)), m_sink(sink),
        m_transitions(std::move(transitions))
    {}

    Automata(const Automata& other):
        m_current(other.m_current), m_initial(other.m_initial),
        m_accepting(other.m_accepting), m_sink(other.m_sink),
        m_transitions(other.m_transitions)
    {}

    Automata(Automata&& other) noexcept:
        m_current(other.m_current), m_initial(other.m_initial),
        m_accepting(std::move(other.m_accepting)), m_sink(other.m_sink),
        m_transitions(std::move(other.m_transitions))
    {}

    // Build the simple transition q0 --symbol--> q1 (accept)
    Automata(const T& s):
        m_current(0), m_initial(0),
        m_accepting({1}), m_sink(2),
        m_transitions({
            {{s, 1}},  // q0 --s--> q1 (accept)
            {},        // q1 accept
            {}         // q2 sink
        })
    {}

    // Build a chain q0 --s[0]--> q1 ... qn (accept), qn+1 (sink)
    template<std::ranges::input_range Range>
        requires std::same_as<std::ranges::range_value_t<Range>, T>
    Automata(const Range& range) {
        StateInd state = 0;
        for (const T& s : range) {
            this->m_transitions.push_back({{s, state + 1}});
            ++state;
        }
        this->m_transitions.push_back({});  // qn accept
        this->m_transitions.push_back({});  // qn+1 sink
        this->m_initial   = 0;
        this->m_current   = 0;
        this->m_accepting = {state};
        this->m_sink      = state + 1;
    }


    size_t nbstates() const {
        return this->m_transitions.size();
    }

    // reset to initial state
    void reset() override {
        m_current = m_initial;
    }

    // Are we in an accepting state ?
    bool accepts() const override {
        return this->m_accepting.contains(this->m_current);
    }

    // Process symbol and move forward to eventual new state
    void process(const T& s) {
        auto& row = this->m_transitions[this->m_current];
        auto it = row.find(s);
        this->m_current = (it != row.end()) ? it->second : this->m_sink;
    }

    template<std::ranges::input_range Range>
        requires std::same_as<std::ranges::range_value_t<Range>, T>
    void process(const Range& range) {
        for (const T& s : range)
            this->process(s);
    }

    // Union operator: accepts if either automaton accepts
    // https://www.geeksforgeeks.org/theory-of-computation/union-process-in-dfa/
    // https://www.geeksforgeeks.org/theory-of-computation/operations-on-dfa/
    friend Automata operator||(const Automata& dfa1, const Automata& dfa2) {
        // Let DFA₁ = (Q₁, δ₁, q₀₁, F₁) and DFA₂ = (Q₂, δ₂, q₀₂, F₂)
        // Then for DFA3
        // States: Q₁ × Q₂
        // Initial state: (q₀₁, q₀₂)
        // Transitions: δ((p, q), a) = (δ₁(p, a), δ₂(q, a))
        // Accepting states: {(p, q) | p ∈ F₁ or q ∈ F₂}
        const size_t dfa3_nbstates = dfa1.nbstates() * dfa2.nbstates();
        const StateInd dfa3_initial = 0;

        // dfa3_transitions[0] will be the initial state of DFA3, representing the pair (q₀₁, q₀₂)
        // More generally, the state pair (i, j) will be stored at dfa3_transitions[i * dfa2.nbstates() + j] as a row-major layout
        // So (0, 0) → 0, (0, 1) → 1, ..., (1, 0) → |Q₂|, (1, 1) → |Q₂| + 1
        std::vector<std::unordered_map<T, StateInd>> dfa3_transitions(dfa3_nbstates);
        const auto df3_ind = [&](StateInd i, StateInd j) { return i * dfa2.nbstates() + j; };
        // Merged sink: both machines are in their respective sinks simultaneously
        const StateInd dfa3_sink = df3_ind(dfa1.m_sink, dfa2.m_sink);
        // Accepting states: {(i, j) | i ∈ F₁ or j ∈ F₂}
        std::set<StateInd> dfa3_accepting;

        std::set<T> symbols;
        for (StateInd i = 0; i < dfa1.nbstates(); ++i) {
            auto const& dfa1_transitions = dfa1.m_transitions[i];
            for (StateInd j = 0; j < dfa2.nbstates(); ++j) {
                auto const& dfa2_transitions = dfa2.m_transitions[j];

                // If either state is accepting, then the superposed state also is
                if (dfa1.m_accepting.contains(i) || dfa2.m_accepting.contains(j)) {
                    dfa3_accepting.insert(df3_ind(i, j));
                }

                // For state (i,j) lets find it possible transitions
                // Lets find all symbols that allow transitionning out of states i AND j
                symbols.clear();
                for (auto const& [symbl, i_next]: dfa1_transitions) {
                    symbols.insert(symbl);
                }
                for (auto const& [symbl, j_next]: dfa2_transitions) {
                    symbols.insert(symbl);
                }
                // Now, lets find new transition for the symbols in DF3
                for (auto const& symbl: symbols) {
                    auto it1 = dfa1_transitions.find(symbl);
                    auto it2 = dfa2_transitions.find(symbl);
                    const StateInd p_next = (it1 != dfa1_transitions.end()) ? it1->second : dfa1.m_sink;
                    const StateInd q_next = (it2 != dfa2_transitions.end()) ? it2->second : dfa2.m_sink;
                    // Only truly sinks when both sides have hit their sinks
                    const StateInd next = (p_next == dfa1.m_sink && q_next == dfa2.m_sink)
                        ? dfa3_sink
                        : df3_ind(p_next, q_next);
                    dfa3_transitions[df3_ind(i, j)][symbl] = next;
                }
            }
        }

        // TODO: Minimize automata
        return Automata(dfa3_initial, std::move(dfa3_accepting), dfa3_sink, std::move(dfa3_transitions));
    }

    // Concatenation operator (dfa1 + dfa2 != dfa2 + dfa1)
    friend Automata operator+(const Automata& dfa1, const Automata& dfa2) {
        // All accepting states of DFA1 needs to be merged with the starting state of DFA2
        // This removes the needs for: the starting state of DFA2, and we also merge both sinks into one
        const size_t dfa3_nbstates = dfa1.nbstates() + dfa2.nbstates() - 2;
        std::vector<std::unordered_map<T, StateInd>> dfa3_transitions(dfa3_nbstates);
        const StateInd dfa3_initial = dfa1.m_initial;
        const StateInd dfa3_sink = dfa1.m_sink;  // reuse dfa1's sink slot (already allocated, left empty)

        // Copy dfa1's transitions into dfa3 (1:1 index mapping), remapping its sink to dfa3_sink
        for (StateInd i = 0; i < dfa1.nbstates(); ++i) {
            if (i == dfa1.m_sink) continue;
            for (auto const& [symbl, target] : dfa1.m_transitions[i])
                dfa3_transitions[i][symbl] = (target == dfa1.m_sink) ? dfa3_sink : target;
        }

        const auto dfa2_offset = [&](StateInd j) -> StateInd {
            return dfa1.nbstates() + j
                 - (dfa2.m_initial < j ? 1 : 0)
                 - (dfa2.m_sink    < j ? 1 : 0);
        };

        // Lets get the initial transition of DFA2
        auto const& dfa2_initial_transitions = dfa2.m_transitions[dfa2.m_initial];

        // Let's plug the accepting states of dfa1 (also stored in dfa3) into DFA2 starting targets
        for (auto const& dfa1_accept: dfa1.m_accepting) {
            // Tricky part, if `dfa1_accept` as an outgoing transition with symbol `a`
            // And dfa2_initial_transitions also have symbol `a`, we need to merge the destination states of dfa2 into the accepting states of dfa1
            /*
            DFA1:  q₀₁ --a--> ... --z--> f₁ (accepting)
                                            │
                                    merge q₀₂ here
                                            │
            DFA2:              q₀₂ --x--> q' --y--> f₂ (accepting)

            DFA3:  q₀₁ --a--> ... --z--> f₁ --x--> q' --y--> f₂ (accepting)
            */
            for (auto const& [symbl, target] : dfa2_initial_transitions) {
                // We will not keep `dfa2.m_sink`
                const StateInd next = (target == dfa2.m_sink) ? dfa3_sink : dfa2_offset(target);  
                auto it1 = dfa1.m_transitions[dfa1_accept].find(symbl);
                // Can dfa1 also transition with `symbl` from this accepting state ?
                if (it1 != dfa1.m_transitions[dfa1_accept].end()) {
                    // Conflict: dfa1 already has a transition on `symbl` from this accepting state,
                    // AND dfa2's initial state also transitions on `symbl`.
                    // A pure DFA cannot represent both choices simultaneously — the correct general
                    // solution requires subset construction to create a merged state {it1->second, next}.
                    StateInd new_state = dfa3_transitions.size();

                    // TODO: implement subset construction for the conflicting case
                } else {
                    // Simple case: add a new transition dfa1_accept --symbl--> next
                    dfa3_transitions[dfa1_accept][symbl] = next;
                }
            }
        }

        // Copy dfa2's remaining states (excluding initial and sink) into dfa3 with offset
        for (StateInd j = 0; j < dfa2.nbstates(); ++j) {
            if (j == dfa2.m_initial || j == dfa2.m_sink) continue;
            for (auto const& [symbl, target] : dfa2.m_transitions[j]) {
                dfa3_transitions[dfa2_offset(j)][symbl] = (target == dfa2.m_sink) ? dfa3_sink : dfa2_offset(target);
            }
        }

        // DFA3 accepting states are dfa2's accepting states remapped through dfa2_offset
        std::set<StateInd> dfa3_accepting;
        for (auto const& dfa2_accept: dfa2.m_accepting) {
            dfa3_accepting.insert(dfa2_offset(dfa2_accept));
        }

        // TODO: Minimize automata
        return Automata(dfa3_initial, std::move(dfa3_accepting), dfa3_sink, std::move(dfa3_transitions));
    }

protected:
    // Current active state
    StateInd m_current;
    // Index of the unique initial state
    StateInd m_initial;
    // Set of accepting states
    std::set<StateInd> m_accepting;
    // Index of the unique sink/trash state
    StateInd m_sink;
    // Transition table for each state: |m_transitions| = |m_states|
    std::vector<std::unordered_map<T, StateInd>> m_transitions;
};

template<Hashable T>
class StringAutomata : public Automata<T> {
public:
    using Automata<T>::Automata;
};

} // namespace blast::core
