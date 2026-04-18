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

// Forward declaration
template<Hashable T> class Automata;

// Non-Deterministic Finite State Automata (NFA)
// Used as a construction intermediate only — convert to DFA via to_dfa()
template<Hashable T>
class NFA {
public:
    NFA(
        std::set<StateInd> initials, std::set<StateInd> acceptings,
        StateInd sink,
        std::vector<std::unordered_map<T, std::set<StateInd>>> transitions = {},
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
            {{s, {1}}},   // q0 --s--> {q1} (accept)
            {},           // q1 accept
            {}            // q2 sink
        }),
        m_epsilon(3)   // one empty ε-set per state
    {}

    // Lift a DFA into an NFA: copies transitions verbatim, epsilon stays empty
    explicit NFA(const Automata<T>& dfa);
    explicit NFA(Automata<T>&& dfa);

    size_t nbstates() const {
        return this->m_transitions.size();
    }

    // Union operator: accepts if either automaton accepts
    friend NFA operator||(const NFA& nfa1, const NFA& nfa2) {
        // Given NFA₁ = (Q₁, δ₁, I₁, F₁) and NFA₂ = (Q₂, δ₂, I₂, F₂), build NFA₃
        const size_t nfa3_nbstates = nfa1.nbstates() * nfa2.nbstates();
        // Map the state pair/superposition: (i, j) will be stored at nfa3_transitions[i * nfa2.nbstates() + j] as a row-major layout
        // So (0, 0) → 0, (0, 1) → 1, ..., (1, 0) → |Q₂|, (1, 1) → |Q₂| + 1
        std::vector<std::unordered_map<T, std::set<StateInd>>> nfa3_transitions(nfa3_nbstates);
        const auto nf3_ind = [&](StateInd i, StateInd j) { return i * nfa2.nbstates() + j; };
        // Merged sink: both machines are in their respective sinks simultaneously
        const StateInd nfa3_sink = nf3_ind(nfa1.m_sink, nfa2.m_sink);

        // nfa3 is in itial state if either of its superposed states of nfa1 or nfa2 are in their initial states
        std::set<StateInd> nfa3_initials, nfa3_acceptings;
        for (const StateInd& i: nfa1.m_initials) {
            for (const StateInd& j: nfa2.m_initials) {
                nfa3_initials.insert(nf3_ind(i, j));
            }
        }

        std::set<T> symbols;
        // epsilon transitions of (i,j): remap each component's epsilon-targets to product indices
        std::vector<std::set<StateInd>> nfa3_epsilon(nfa3_nbstates);
        for (StateInd i = 0; i < nfa1.nbstates(); ++i) {
            auto const& nfa1_transitions = nfa1.m_transitions[i];
            for (StateInd j = 0; j < nfa2.nbstates(); ++j) {
                auto const& nfa2_transitions = nfa2.m_transitions[j];

                // If either state is accepting, then the superposed state also is
                if (nfa1.m_acceptings.contains(i) || nfa2.m_acceptings.contains(j)) {
                    nfa3_acceptings.insert(nf3_ind(i, j));
                }

                for (const StateInd i_eps : nfa1.m_epsilon[i]) {
                    nfa3_epsilon[nf3_ind(i, j)].insert(nf3_ind(i_eps, j));
                }
                for (const StateInd j_eps : nfa2.m_epsilon[j]) {
                    nfa3_epsilon[nf3_ind(i, j)].insert(nf3_ind(i, j_eps));
                }
                // For state (i,j) lets find it possible transitions
                // Lets find all symbols that allow transitionning out of states i AND j
                symbols.clear();
                for (auto const& [symbl, i_next]: nfa1_transitions) {
                    symbols.insert(symbl);
                }
                for (auto const& [symbl, j_next]: nfa2_transitions) {
                    symbols.insert(symbl);
                }
                // Now, lets find new transition for the symbols in DF3
                for (auto const& symbl: symbols) {
                    auto it1 = nfa1_transitions.find(symbl);
                    auto it2 = nfa2_transitions.find(symbl);
                    const std::set<StateInd> i_nexts = (it1 != nfa1_transitions.end()) ? it1->second : std::set<StateInd>{nfa1.m_sink};
                    const std::set<StateInd> j_nexts = (it2 != nfa2_transitions.end()) ? it2->second : std::set<StateInd>{nfa2.m_sink};
                    // Cartesian product: all (i', j') pairs reachable from (i, j) on symbl
                    for (const StateInd i_next : i_nexts) {
                        for (const StateInd j_next : j_nexts) {
                            // Only truly sinks when both sides have hit their sinks
                            const StateInd next = (i_next == nfa1.m_sink && j_next == nfa2.m_sink)
                                ? nfa3_sink
                                : nf3_ind(i_next, j_next);
                            nfa3_transitions[nf3_ind(i, j)][symbl].insert(next);
                        }
                    }
                }
            }
        }
        return NFA(std::move(nfa3_initials), std::move(nfa3_acceptings), nfa3_sink, std::move(nfa3_transitions), std::move(nfa3_epsilon));
    }

    // Concatenation operator (nfa1 + nfa2 != nfa2 + nfa1)
    friend NFA operator+(const NFA& nfa1, const NFA& nfa2) {
        const size_t n1 = nfa1.nbstates();
        const size_t n2 = nfa2.nbstates();
        // Reuse nfa1's sink as nfa3's sink; remap nfa2.m_sink to it
        const StateInd nfa3_sink = nfa1.m_sink;
        const auto nfa2_ind = [&](StateInd j) {
            return (j == nfa2.m_sink) ? nfa3_sink : n1 + j;
        };

        // Copy nfa1 transitions verbatim, then nfa2 with offset + sink remap
        std::vector<std::unordered_map<T, std::set<StateInd>>> nfa3_transitions(n1 + n2);
        for (StateInd i = 0; i < n1; ++i) {
            nfa3_transitions[i] = nfa1.m_transitions[i];
        }
        for (StateInd j = 0; j < n2; ++j) {
            for (auto const& [symbl, j_nexts] : nfa2.m_transitions[j]) {
                for (StateInd j_next : j_nexts) {
                    nfa3_transitions[nfa2_ind(j)][symbl].insert(nfa2_ind(j_next));
                }
            }
        }

        // Copy epsilon transitions; plug nfa1 accepting states -> nfa2 initial states
        std::vector<std::set<StateInd>> nfa3_epsilon(n1 + n2);
        for (StateInd i = 0; i < n1; ++i) {
            nfa3_epsilon[i] = nfa1.m_epsilon[i];
        }
        for (StateInd j = 0; j < n2; ++j) {
            for (StateInd j_eps : nfa2.m_epsilon[j]) {
                nfa3_epsilon[nfa2_ind(j)].insert(nfa2_ind(j_eps));
            }
        }
        for (const StateInd f : nfa1.m_acceptings) {
            for (const StateInd i : nfa2.m_initials) {
                nfa3_epsilon[f].insert(nfa2_ind(i));
            }
        }

        std::set<StateInd> nfa3_acceptings;
        for (const StateInd f : nfa2.m_acceptings) {
            nfa3_acceptings.insert(nfa2_ind(f));
        }

        return NFA(nfa1.m_initials, std::move(nfa3_acceptings), nfa3_sink, std::move(nfa3_transitions), std::move(nfa3_epsilon));
    }

    // Kleen + (1 or more occurences) operator
    friend NFA operator+(const NFA& nfa) {
        // All accepting now have epsilon-transitions to the starting states
        NFA nfa2(nfa);
        // Add all epsilon transitions
        for(const StateInd& i: nfa2.m_acceptings) {
            nfa2.m_epsilon[i].insert(nfa2.m_initials.begin(), nfa2.m_initials.end());
        }
        return nfa2;
    }

    // Kleen star (0 or more occurences) operator
    friend NFA operator*(const NFA& nfa) {
        // staring an NFA is easy: all accepting now have epsilon-transitions to the starting states
        // Starting states are now also accepting
        NFA nfa2 = +nfa;
        nfa2.m_acceptings.insert(nfa2.m_initials.begin(), nfa2.m_initials.end());
        return nfa2;
    }

    // Optionnal operator
    // If nfa 'N' recognize the language 'L', then '~N' recognize {L || empty}
    // [0,1] basically
    friend NFA operator~(const NFA& nfa) {
        // Starting states are now also accepting
        NFA nfa2(nfa);
        nfa2.m_acceptings.insert(nfa2.m_initials.begin(), nfa2.m_initials.end());
        return nfa2;
    }
    

    // https://www.geeksforgeeks.org/theory-of-computation/conversion-from-nfa-to-dfa/
    Automata<T> deterministic() const {
        // Given a state `q`, return the epsilon closure of `q`
        // Meaning the set of ALL states reachable from `q` by following ONLY epsilon-transitions
        // For example: q --ε--> p --ε--> r := {p,r}
        auto simple_epsilon_closure = [&](const StateInd& q) {
            std::set<StateInd> closure = this->m_epsilon[q];
            std::vector<StateInd> worklist(closure.begin(), closure.end());
            while (!worklist.empty()) {
                StateInd s = worklist.back(); worklist.pop_back();
                for (StateInd eps : m_epsilon[s]) {
                    if (closure.insert(eps).second) {
                        worklist.push_back(eps);
                    }
                }
            }
            return closure;
        };

        // Given a superposed state 'qs', give its epsilon-closure
        // Includes the input states themselves (trivially reachable)
        auto epsilon_closure = [&](const std::set<StateInd>& qs) {
            std::set<StateInd> closure = qs;
            for (const StateInd& q : qs) {
                auto sub = simple_epsilon_closure(q);
                closure.insert(sub.begin(), sub.end());
            }
            return closure;
        };

        struct SetHash {
            size_t operator()(const std::set<StateInd>& s) const {
                size_t seed = s.size();
                for (StateInd v : s)
                    seed ^= std::hash<StateInd>{}(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
                return seed;
            }
        };

        // Initial state of the DFA -> Superposition of the epsilon_closure of the NFA's initial states
        std::set<StateInd> initial = epsilon_closure(this->m_initials);
        std::unordered_map<std::set<StateInd>, std::unordered_map<T, std::set<StateInd>>, SetHash> superposed_states_transitions;
        std::vector<std::set<StateInd>> states_stack;
        states_stack.push_back(initial);

        std::unordered_map<T, std::set<StateInd>> buffer_transitions;
        while (!states_stack.empty()) {
            auto const qs = states_stack.back(); states_stack.pop_back();
            // Did we already process this superposed state?
            if (superposed_states_transitions.find(qs) != superposed_states_transitions.end()) {
                continue;
            }

            // Collect raw NFA targets for each symbol across all states in qs
            buffer_transitions.clear();
            for (const StateInd& q : qs) {
                for (auto const& [symbl, q_nexts] : this->m_transitions[q]) {
                    buffer_transitions[symbl].insert(q_nexts.begin(), q_nexts.end());
                }
            }

            // Apply epsilon closure to each raw target set → new DFA superposed state
            auto& row = superposed_states_transitions[qs];
            for (auto& [symbl, raw_nexts] : buffer_transitions) {
                std::set<StateInd> next_qs = epsilon_closure(raw_nexts);
                if (superposed_states_transitions.find(next_qs) == superposed_states_transitions.end()) {
                    states_stack.push_back(next_qs);
                }
                row[symbl] = std::move(next_qs);
            }
        }

        // Now, build the DFA from `superposed_states_transitions`
        // Assign a contiguous index to each discovered superposed state; initial gets 0
        std::unordered_map<std::set<StateInd>, StateInd, SetHash> state_index;
        state_index.reserve(superposed_states_transitions.size() + 1);
        state_index[initial] = 0;
        StateInd idx = 1;
        for (auto const& [qs, _] : superposed_states_transitions) {
            if (state_index.find(qs) == state_index.end())
                state_index[qs] = idx++;
        }
        const StateInd dfa_sink = idx;  // one extra dead state for missing transitions
        std::vector<std::unordered_map<T, StateInd>> dfa_transitions(idx + 1);
        std::set<StateInd> dfa_accepting;

        for (auto const& [qs, sym_map] : superposed_states_transitions) {
            const StateInd from = state_index[qs];
            for (auto const& [symbl, next_qs] : sym_map) {
                auto it = state_index.find(next_qs);
                dfa_transitions[from][symbl] = (it != state_index.end()) ? it->second : dfa_sink;
            }
            for (const StateInd& q : qs) {
                if (this->m_acceptings.contains(q)) {
                    dfa_accepting.insert(from);
                    break;
                }
            }
        }

        return Automata<T>(state_index[initial], std::move(dfa_accepting), dfa_sink, std::move(dfa_transitions));
    }

protected:
    // Index of the initial states
    std::set<StateInd> m_initials;
    // Set of accepting states
    std::set<StateInd> m_acceptings;
    // Index of the unique sink/trash state
    StateInd m_sink;
    // Transition table for each state: |m_transitions| = |m_states|
    std::vector<std::unordered_map<T, std::set<StateInd>>> m_transitions;
    // m_epsilon[i] = epsilon-reachable states from i
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

    Automata& operator=(const Automata&) = default;
    Automata& operator=(Automata&&) noexcept = default;

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

    // Empty automata
    Automata():
        m_current(0), m_initial(0),
        m_accepting({}), m_sink(0),
        m_transitions({})
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


    size_t nbstates() const { return m_transitions.size(); }
    const StateInd& current() const { return m_current; }
    const StateInd& sink() const { return m_sink; }
    // Given a state, return the outbound transitions
    const std::unordered_map<T, StateInd>& out_transitions(const StateInd& q) const {
        if (q >= this->m_transitions.size())
            throw std::out_of_range("state index out of range");
        return this->m_transitions[q];
    }

    // Force transition
    void force_state(const StateInd& q) { 
        if (q >= this->m_transitions.size())
            throw std::out_of_range("state index out of range");
        this->m_current = q;
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
        for (const T& s : range) {
            this->process(s);
        }
    }

    // Union operator: accepts if either automaton accepts
    // https://www.geeksforgeeks.org/theory-of-computation/union-process-in-dfa/
    // https://www.geeksforgeeks.org/theory-of-computation/operations-on-dfa/
    friend Automata operator||(const Automata& dfa1, const Automata& dfa2) {
        return (NFA<T>(dfa1) || NFA<T>(dfa2)).deterministic();
    }

    // Concatenation operator (dfa1 + dfa2 != dfa2 + dfa1)
    friend Automata operator+(const Automata& dfa1, const Automata& dfa2) {
       return (NFA<T>(dfa1) + NFA<T>(dfa2)).deterministic();
    }

    // Kleen + (1 or more occurences) operator
    friend Automata operator+(const Automata& dfa) {
        return (+(NFA<T>(dfa))).deterministic();
    }

    // Kleen star (0 or more occurences) operator
    friend Automata operator*(const Automata& dfa) {
        return (*(NFA<T>(dfa))).deterministic();
    }

    // Optionnal operator
    // If nfa 'N' recognize the language 'L', then '~N' recognize {L || empty}
    // [0,1] basically
    friend Automata operator~(const Automata& dfa) {
        return (~(NFA<T>(dfa))).deterministic();
    }

    friend class NFA<T>;
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
NFA<T>::NFA(const Automata<T>& dfa)
    : m_initials({dfa.m_initial}),
      m_acceptings(dfa.m_accepting),
      m_sink(dfa.m_sink),
      m_epsilon(dfa.m_transitions.size())
{
    m_transitions.reserve(dfa.m_transitions.size());
    for (const auto& row : dfa.m_transitions) {
        std::unordered_map<T, std::set<StateInd>> nfa_row;
        for (const auto& [sym, target] : row)
            nfa_row[sym] = {target};
        m_transitions.push_back(std::move(nfa_row));
    }
}

template<Hashable T>
NFA<T>::NFA(Automata<T>&& dfa)
    : m_initials({dfa.m_initial}),
      m_acceptings(std::move(dfa.m_accepting)),
      m_sink(dfa.m_sink),
      m_epsilon(dfa.m_transitions.size())
{
    m_transitions.reserve(dfa.m_transitions.size());
    for (auto& row : dfa.m_transitions) {
        std::unordered_map<T, std::set<StateInd>> nfa_row;
        for (auto& [sym, target] : row)
            nfa_row[std::move(sym)] = {target};
        m_transitions.push_back(std::move(nfa_row));
    }
}

} // namespace blast::core
