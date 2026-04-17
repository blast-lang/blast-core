#include <catch2/catch_test_macros.hpp>
#include <core/parser/Regex.hpp>

using namespace blast::core;
using namespace blast::core::parser;

// DFA recognizing exactly U"abc":
// q0 --a--> q1 --b--> q2 --c--> q3 (accept)
// any other symbol -> sink (q4)
static StringAutomata<char32_t> make_abc_automata() {
    return StringAutomata<char32_t>(
        /*initial=*/  0,
        /*accepting=*/{3},
        /*sink=*/     4,
        /*transitions=*/{
            {{SymblRange{U'a'}, 1}},  // q0
            {{SymblRange{U'b'}, 2}},  // q1
            {{SymblRange{U'c'}, 3}},  // q2
            {},                        // q3 (accepting)
            {},                        // q4 (sink)
        }
    );
}

TEST_CASE("StringAutomata<char32_t> accepts exact string", "[automata]") {
    auto dfa = make_abc_automata();
    dfa.process(std::u32string(U"abc"));
    REQUIRE(dfa.accepts());
}

TEST_CASE("StringAutomata<char32_t> rejects partial input", "[automata]") {
    auto dfa = make_abc_automata();
    dfa.process(std::u32string(U"ab"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("StringAutomata<char32_t> rejects wrong input", "[automata]") {
    auto dfa = make_abc_automata();
    dfa.process(std::u32string(U"xyz"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("StringAutomata<char32_t> resets correctly", "[automata]") {
    auto dfa = make_abc_automata();
    dfa.process(std::u32string(U"abc"));
    REQUIRE(dfa.accepts());
    dfa.reset();
    REQUIRE_FALSE(dfa.accepts());
    dfa.process(std::u32string(U"abc"));
    REQUIRE(dfa.accepts());
}

// Range constructor: builds chain q0 --s[0]--> ... --> qn (accept)
static StringAutomata<char32_t> make_abc_range_automata() {
    return StringAutomata<char32_t>(std::u32string(U"abc"));
}

TEST_CASE("Range ctor: accepts full input", "[automata][range_ctor]") {
    auto dfa = make_abc_range_automata();
    dfa.process(std::u32string(U"abc"));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Range ctor: does not accept partial input", "[automata][range_ctor]") {
    auto dfa = make_abc_range_automata();
    dfa.process(std::u32string(U"ab"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Range ctor: does not accept wrong input", "[automata][range_ctor]") {
    auto dfa = make_abc_range_automata();
    dfa.process(std::u32string(U"xyz"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Range ctor: resets correctly", "[automata][range_ctor]") {
    auto dfa = make_abc_range_automata();
    dfa.process(std::u32string(U"abc"));
    REQUIRE(dfa.accepts());
    dfa.reset();
    REQUIRE_FALSE(dfa.accepts());
    dfa.process(std::u32string(U"abc"));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Range ctor: single-symbol range", "[automata][range_ctor]") {
    StringAutomata<char32_t> dfa(std::u32string(U"x"));
    dfa.process(std::u32string(U"x"));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Range ctor: single-symbol rejects wrong symbol", "[automata][range_ctor]") {
    StringAutomata<char32_t> dfa(std::u32string(U"x"));
    dfa.process(std::u32string(U"y"));
    REQUIRE_FALSE(dfa.accepts());
}

// Union (operator||) tests
// Recognizes "abc" OR "xyz"
static StringAutomata<char32_t> make_abc_or_xyz() {
    return StringAutomata<char32_t>(std::u32string(U"abc"))
        || StringAutomata<char32_t>(std::u32string(U"xyz"));
}

TEST_CASE("Union: accepts left-side word", "[automata][union]") {
    auto dfa = make_abc_or_xyz();
    dfa.process(std::u32string(U"abc"));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Union: accepts right-side word", "[automata][union]") {
    auto dfa = make_abc_or_xyz();
    dfa.process(std::u32string(U"xyz"));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Union: rejects word matching neither side", "[automata][union]") {
    auto dfa = make_abc_or_xyz();
    dfa.process(std::u32string(U"def"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Union: rejects partial left-side word", "[automata][union]") {
    auto dfa = make_abc_or_xyz();
    dfa.process(std::u32string(U"ab"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Union: rejects partial right-side word", "[automata][union]") {
    auto dfa = make_abc_or_xyz();
    dfa.process(std::u32string(U"xy"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Union: resets correctly", "[automata][union]") {
    auto dfa = make_abc_or_xyz();
    dfa.process(std::u32string(U"abc"));
    REQUIRE(dfa.accepts());
    dfa.reset();
    REQUIRE_FALSE(dfa.accepts());
    dfa.process(std::u32string(U"xyz"));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Union: single-symbol each side", "[automata][union]") {
    auto dfa = StringAutomata<char32_t>(std::u32string(U"a"))
            || StringAutomata<char32_t>(std::u32string(U"b"));

    dfa.process(std::u32string(U"a"));
    REQUIRE(dfa.accepts());
    dfa.reset();

    dfa.process(std::u32string(U"b"));
    REQUIRE(dfa.accepts());
    dfa.reset();

    dfa.process(std::u32string(U"c"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Union: is commutative", "[automata][union]") {
    auto lhs = StringAutomata<char32_t>(std::u32string(U"abc"))
            || StringAutomata<char32_t>(std::u32string(U"xyz"));
    auto rhs = StringAutomata<char32_t>(std::u32string(U"xyz"))
            || StringAutomata<char32_t>(std::u32string(U"abc"));

    for (const char32_t* s : {U"abc", U"xyz", U"def"}) {
        std::u32string input(s);
        lhs.process(input);
        rhs.process(input);
        REQUIRE(lhs.accepts() == rhs.accepts());
        lhs.reset();
        rhs.reset();
    }
}

// Concatenation (operator+) tests
// Recognizes "abc" followed by "xyz" → "abcxyz"
static StringAutomata<char32_t> make_abc_then_xyz() {
    return StringAutomata<char32_t>(std::u32string(U"abc"))
         + StringAutomata<char32_t>(std::u32string(U"xyz"));
}

TEST_CASE("Concat: accepts full concatenated input", "[automata][concat]") {
    auto dfa = make_abc_then_xyz();
    dfa.process(std::u32string(U"abcxyz"));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Concat: rejects first word alone", "[automata][concat]") {
    auto dfa = make_abc_then_xyz();
    dfa.process(std::u32string(U"abc"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Concat: rejects second word alone", "[automata][concat]") {
    auto dfa = make_abc_then_xyz();
    dfa.process(std::u32string(U"xyz"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Concat: rejects reversed order", "[automata][concat]") {
    auto dfa = make_abc_then_xyz();
    dfa.process(std::u32string(U"xyzabc"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Concat: rejects partial first word", "[automata][concat]") {
    auto dfa = make_abc_then_xyz();
    dfa.process(std::u32string(U"abcxy"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Concat: rejects wrong input", "[automata][concat]") {
    auto dfa = make_abc_then_xyz();
    dfa.process(std::u32string(U"abcdef"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Concat: resets correctly", "[automata][concat]") {
    auto dfa = make_abc_then_xyz();
    dfa.process(std::u32string(U"abcxyz"));
    REQUIRE(dfa.accepts());
    dfa.reset();
    REQUIRE_FALSE(dfa.accepts());
    dfa.process(std::u32string(U"abcxyz"));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Concat: single-symbol each side", "[automata][concat]") {
    auto dfa = StringAutomata<char32_t>(std::u32string(U"a"))
             + StringAutomata<char32_t>(std::u32string(U"b"));
    dfa.process(std::u32string(U"ab"));
    REQUIRE(dfa.accepts());
    dfa.reset();
    dfa.process(std::u32string(U"a"));
    REQUIRE_FALSE(dfa.accepts());
    dfa.reset();
    dfa.process(std::u32string(U"b"));
    REQUIRE_FALSE(dfa.accepts());
}

// Kleene+ (operator+): one or more occurrences
TEST_CASE("Plus: accepts single occurrence", "[automata][plus]") {
    auto dfa = +StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U"ab"));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Plus: accepts two occurrences", "[automata][plus]") {
    auto dfa = +StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U"abab"));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Plus: accepts three occurrences", "[automata][plus]") {
    auto dfa = +StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U"ababab"));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Plus: rejects empty input", "[automata][plus]") {
    auto dfa = +StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U""));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Plus: rejects partial occurrence", "[automata][plus]") {
    auto dfa = +StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U"a"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Plus: rejects one-and-a-half occurrences", "[automata][plus]") {
    auto dfa = +StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U"aba"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Plus: resets correctly", "[automata][plus]") {
    auto dfa = +StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U"abab"));
    REQUIRE(dfa.accepts());
    dfa.reset();
    REQUIRE_FALSE(dfa.accepts());
    dfa.process(std::u32string(U"ab"));
    REQUIRE(dfa.accepts());
}

// Kleene* (operator*): zero or more occurrences
TEST_CASE("Star: accepts empty input", "[automata][star]") {
    auto dfa = *StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U""));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Star: accepts single occurrence", "[automata][star]") {
    auto dfa = *StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U"ab"));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Star: accepts two occurrences", "[automata][star]") {
    auto dfa = *StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U"abab"));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Star: rejects partial occurrence", "[automata][star]") {
    auto dfa = *StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U"a"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Star: rejects one-and-a-half occurrences", "[automata][star]") {
    auto dfa = *StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U"aba"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Star: resets correctly", "[automata][star]") {
    auto dfa = *StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U"abab"));
    REQUIRE(dfa.accepts());
    dfa.reset();
    REQUIRE(dfa.accepts());  // initial state accepts (empty string)
    dfa.process(std::u32string(U"ab"));
    REQUIRE(dfa.accepts());
}

// Optional (operator~): zero or one occurrence
TEST_CASE("Optional: accepts empty input", "[automata][optional]") {
    auto dfa = ~StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U""));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Optional: accepts single occurrence", "[automata][optional]") {
    auto dfa = ~StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U"ab"));
    REQUIRE(dfa.accepts());
}

TEST_CASE("Optional: rejects two occurrences", "[automata][optional]") {
    auto dfa = ~StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U"abab"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Optional: rejects partial occurrence", "[automata][optional]") {
    auto dfa = ~StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U"a"));
    REQUIRE_FALSE(dfa.accepts());
}

TEST_CASE("Optional: resets correctly", "[automata][optional]") {
    auto dfa = ~StringAutomata<char32_t>(std::u32string(U"ab"));
    dfa.process(std::u32string(U"ab"));
    REQUIRE(dfa.accepts());
    dfa.reset();
    REQUIRE(dfa.accepts());  // initial state accepts (empty string)
    dfa.process(std::u32string(U"ab"));
    REQUIRE(dfa.accepts());
}
