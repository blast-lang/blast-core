#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <variant> // std::monostate

namespace blast::core {

using StateInd = size_t;
class IAutomata {
public:
    virtual ~IAutomata() = default;
    virtual void reset() = 0;
    virtual bool accepts() const = 0;
};

// Deterministic finite state automata with fixed number of states
// https://en.wikipedia.org/wiki/Deterministic_finite_automaton
// https://cs.wellesley.edu/~cs235/fall10/lectures/14_DFA_operations_revised_2.pdf
template<typename SymbolT>
class Automata : public IAutomata {
public:
    Automata(
        StateInd start, StateInd accepting,
        StateInd sink,
        std::vector<std::unordered_map<SymbolT, StateInd>> transitions = {}
    ): m_current(start), m_start(start),
        m_accepting(accepting), m_sink(sink),
        m_transitions(transitions)
    {}

    Automata(const Automata& other): 
        m_current(other.m_current), m_start(other.m_start),
        m_accepting(other.m_accepting), m_sink(other.m_sink),
        m_transitions(other.m_transitions)
    {}

    Automata(Automata&& other) noexcept:
        m_current(other.m_current), m_start(other.m_start),
        m_accepting(other.m_accepting), m_sink(other.m_sink),
        m_transitions(std::move(other.m_transitions))
    {}

    // Build the simple transition start --symbol--> accept
    Automata(const SymbolT& s):
        m_current(0), m_start(0),
        m_accepting(1), m_sink(2),
        m_transitions({{s, 1}})
    {}

    // reset to initial state
    void reset() override {
        m_current = m_start;
    }

    // Are we in an acception state ?
    bool accepts() const override {
        return (this->m_current != this->m_sink) && (this->m_current == this->m_accepting);
    }

    // Process symbol and move forward to eventual new state
    void process(const SymbolT& s) {
        auto& row = this->m_transitions[this->m_current];
        auto it = row.find(s);
        this->m_current = (it != row.end()) ? it->second : this->m_sink;
    }

    // Union: accepts if either automaton accepts
    // https://www.geeksforgeeks.org/theory-of-computation/union-process-in-dfa/
    friend Automata operator||(const Automata& d1, const Automata& d2) {
        
    }

protected:
    // Current active state
    StateInd m_current;
    // Index of the unique starting state
    StateInd m_start;
    // Index of the unique accepting state
    StateInd m_accepting;
    // Index of the unique sink/trash state
    StateInd m_sink;
    // Transition table for each states: |m_transitions| = |m_states|
    std::vector<std::unordered_map<SymbolT, StateInd>> m_transitions;
};

class UFT32StringAutomata: public Automata<char32_t> {
public:
    using Automata<char32_t>::Automata;
};

} // namespace blast::core
