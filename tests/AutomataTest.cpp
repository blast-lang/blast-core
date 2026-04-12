#include <catch2/catch_test_macros.hpp>
#include <core/Automata.hpp>

using namespace blast::core;

// DFA recognizing exactly U"abc":
// q0 --a--> q1 --b--> q2 --c--> q3 (accept)
// any other symbol -> sink (q4)
static UFT32StringAutomata make_abc_automata() {
    return UFT32StringAutomata(
        /*start=*/    0,
        /*accepting=*/3,
        /*sink=*/     4,
        /*transitions=*/{
            {{U'a', 1}},          // q0
            {{U'b', 2}},          // q1
            {{U'c', 3}},          // q2
            {},                   // q3 (accepting)
            {},                   // q4 (sink)
        }
    );
}

TEST_CASE("UFT32StringAutomata accepts exact string", "[automata]") {
    auto dfa = make_abc_automata();
    for (char32_t c : std::u32string(U"abc"))
        dfa.process(c);
    REQUIRE(dfa.accepts());
}

TEST_CASE("UFT32StringAutomata rejects partial input", "[automata]") {
    auto dfa = make_abc_automata();
    for (char32_t c : std::u32string(U"ab"))
        dfa.process(c);
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("UFT32StringAutomata rejects wrong input", "[automata]") {
    auto dfa = make_abc_automata();
    for (char32_t c : std::u32string(U"xyz"))
        dfa.process(c);
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("UFT32StringAutomata resets correctly", "[automata]") {
    auto dfa = make_abc_automata();
    for (char32_t c : std::u32string(U"abc"))
        dfa.process(c);
    REQUIRE(dfa.accepts());
    dfa.reset();
    REQUIRE_FALSE(dfa.accepts());
    for (char32_t c : std::u32string(U"abc"))
        dfa.process(c);
    REQUIRE(dfa.accepts());
}
