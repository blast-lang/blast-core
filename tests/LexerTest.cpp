#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_template_test_macros.hpp>
#include <core/lexer/Lexer.hpp>
#include <core/Exception.hpp>

using namespace blast::core;
using namespace blast::core::lexer;

TEST_CASE("LexError: stores message and position", "[lexerror]") {
    LexError e("unexpected char", 42);
    CHECK(std::string(e.what()) == "unexpected char");
    CHECK(e.position() == 42);
}

TEST_CASE("LexError: default position is zero", "[lexerror]") {
    LexError e("error");
    CHECK(e.position() == 0);
}

TEST_CASE("LexError: inherits from Exception", "[lexerror]") {
    LexError e("msg", 7);
    const Exception& base = e;
    CHECK(std::string(base.what()) == "msg");
}

TEST_CASE("Tokenizer: empty input produces no tokens", "[tokenizer]") {
    Tokenizer t;
    t.process("");
    CHECK(t.tokens().empty());
}

TEST_CASE("Tokenizer: skips whitespace and newlines", "[tokenizer]") {
    Tokenizer t;
    t.process("  \t\n  ");
    CHECK(t.tokens().empty());
}

TEST_CASE("Tokenizer: integer literal", "[tokenizer]") {
    Tokenizer t;
    t.process("42");
    auto const& toks = t.tokens();
    REQUIRE(toks.size() == 1);
    CHECK(toks[0].m_kind == Tokenizer::TokenKind::INT_LIT);
    CHECK(toks[0].m_value == "42");
}

TEST_CASE("Tokenizer: multiple tokens", "[tokenizer]") {
    Tokenizer t;
    t.process("123+456");
    auto const& toks = t.tokens();
    REQUIRE(toks.size() == 3);
    CHECK(toks[0].m_kind == Tokenizer::TokenKind::INT_LIT);
    CHECK(toks[0].m_value == "123");
    CHECK(toks[1].m_kind == Tokenizer::TokenKind::BIN_OP);
    CHECK(toks[1].m_value == "+");
    CHECK(toks[2].m_kind == Tokenizer::TokenKind::INT_LIT);
    CHECK(toks[2].m_value == "456");
}

TEST_CASE("Tokenizer: tokenizes with whitespace separation", "[tokenizer]") {
    Tokenizer t;
    t.process("123 + 456");
    auto const& toks = t.tokens();
    REQUIRE(toks.size() == 3);
    CHECK(toks[0].m_kind == Tokenizer::TokenKind::INT_LIT);
    CHECK(toks[0].m_value == "123");
    CHECK(toks[1].m_kind == Tokenizer::TokenKind::BIN_OP);
    CHECK(toks[1].m_value == "+");
    CHECK(toks[2].m_kind == Tokenizer::TokenKind::INT_LIT);
    CHECK(toks[2].m_value == "456");
}

TEST_CASE("Tokenizer: keyword beats identifier (priority)", "[tokenizer]") {
    Tokenizer t;
    t.process("if");
    auto const& toks = t.tokens();
    REQUIRE(toks.size() == 1);
    CHECK(toks[0].m_kind == Tokenizer::TokenKind::IF_STMT);
    CHECK(toks[0].m_value == "if");
}

TEST_CASE("Tokenizer: identifier does not match keyword", "[tokenizer]") {
    Tokenizer t;
    t.process("iff");
    auto const& toks = t.tokens();
    REQUIRE(toks.size() == 1);
    CHECK(toks[0].m_kind == Tokenizer::TokenKind::IDENDIFIER);
    CHECK(toks[0].m_value == "iff");
}

TEST_CASE("Tokenizer: maximal munch prefers longer match", "[tokenizer]") {
    Tokenizer t;
    t.process("123abc");
    auto const& toks = t.tokens();
    REQUIRE(toks.size() == 2);
    CHECK(toks[0].m_kind == Tokenizer::TokenKind::INT_LIT);
    CHECK(toks[0].m_value == "123");
    CHECK(toks[1].m_kind == Tokenizer::TokenKind::IDENDIFIER);
    CHECK(toks[1].m_value == "abc");
}

TEST_CASE("Tokenizer: float literal", "[tokenizer]") {
    Tokenizer t;
    t.process("3.14");
    auto const& toks = t.tokens();
    REQUIRE(toks.size() == 1);
    CHECK(toks[0].m_kind == Tokenizer::TokenKind::FLOAT_LIT);
    CHECK(toks[0].m_value == "3.14");
}

TEST_CASE("Tokenizer: bool literal", "[tokenizer]") {
    Tokenizer t;
    t.process("true");
    auto const& toks = t.tokens();
    REQUIRE(toks.size() == 1);
    CHECK(toks[0].m_kind == Tokenizer::TokenKind::BOOL_LIT);
    CHECK(toks[0].m_value == "true");
}

TEST_CASE("Tokenizer: string literal", "[tokenizer]") {
    Tokenizer t;
    t.process("\"hello\"");
    auto const& toks = t.tokens();
    REQUIRE(toks.size() == 1);
    CHECK(toks[0].m_kind == Tokenizer::TokenKind::STR_LIT);
    CHECK(toks[0].m_value == "\"hello\"");
}

TEST_CASE("Tokenizer: invalid character throws LexError", "[tokenizer]") {
    Tokenizer t;
    REQUIRE_THROWS_AS(t.process("@"), LexError);
}

TEST_CASE("Tokenizer: LexError has correct position", "[tokenizer]") {
    Tokenizer t;
    t.process("123");
    try {
        t.process("123@");
        FAIL("expected LexError");
    } catch (const LexError& e) {
        CHECK(e.position() == 3);
    }
}

TEST_CASE("Tokenizer: semicolon", "[tokenizer]") {
    Tokenizer t;
    t.process(";");
    auto const& toks = t.tokens();
    REQUIRE(toks.size() == 1);
    CHECK(toks[0].m_kind == Tokenizer::TokenKind::ENDEXPR);
    CHECK(toks[0].m_value == ";");
}

TEST_CASE("Tokenizer: parentheses", "[tokenizer]") {
    Tokenizer t;
    t.process("( )");
    auto const& toks = t.tokens();
    REQUIRE(toks.size() == 2);
    CHECK(toks[0].m_kind == Tokenizer::TokenKind::OPEN_PAR);
    CHECK(toks[1].m_kind == Tokenizer::TokenKind::CLOSE_PAR);
}
