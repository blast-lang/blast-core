#include <catch2/catch_test_macros.hpp>
#include <core/parser/Parser.hpp>
#include <core/utils/Dump.hpp>
#include <core/Exception.hpp>
#include <memory>
#include <string>

using namespace blast::core;
using namespace blast::core::parser;
using blast::core::utils::dump;
using blast::core::lexer::Tokenizer;

// Parse a source string and return the compact s-expression dump of the root.
static std::string parseDump(const std::string& source) {
    Tokenizer tokenizer;
    tokenizer.process(source);
    SimpleParser parser(tokenizer);
    parser.buildAST();
    return dump(parser.root());
}

// --- AST nodes (constructed directly) ------------------------------------

TEST_CASE("dump: int literal", "[ast]") {
    IntLiteral n(42);
    CHECK(dump(&n) == "(int 42)");
}

TEST_CASE("dump: identifier", "[ast]") {
    Identifier n("foo");
    CHECK(dump(&n) == "(id foo)");
}

TEST_CASE("dump: binary expression is nested", "[ast]") {
    auto expr = std::make_unique<BinaryExpr>(
        "+",
        std::make_unique<IntLiteral>(1),
        std::make_unique<IntLiteral>(2));
    CHECK(dump(expr.get()) == "(binary + (int 1) (int 2))");
}

TEST_CASE("dump: assignment", "[ast]") {
    Assign a(std::make_unique<Identifier>("a"), std::make_unique<IntLiteral>(5));
    CHECK(dump(&a) == "(assign (id a) (int 5))");
}

TEST_CASE("dump: var-decl with type and no initializer", "[ast]") {
    VarDecl decl("a", std::make_unique<Identifier>("Int"), nullptr);
    CHECK(dump(&decl) == "(var-decl a :: (id Int))");
}

TEST_CASE("dump: var-decl with type and initializer", "[ast]") {
    VarDecl decl("a",
                 std::make_unique<Identifier>("Int"),
                 std::make_unique<IntLiteral>(5));
    CHECK(dump(&decl) == "(var-decl a :: (id Int) = (int 5))");
}

// --- parser (end to end) --------------------------------------------------

TEST_CASE("parse: empty input yields an empty unit", "[parser]") {
    CHECK(parseDump("") == "(unit)");
}

TEST_CASE("parse: single type declaration", "[parser]") {
    CHECK(parseDump("a::Int;") == "(unit (var-decl a :: (id Int)))");
}

TEST_CASE("parse: multiple declarations preserve order", "[parser]") {
    CHECK(parseDump("a::Int;\nfoo::Float;") ==
          "(unit (var-decl a :: (id Int)) (var-decl foo :: (id Float)))");
}

// An identifier not followed by '::' is not a malformed declaration -- it is
// the start of an expression statement.
TEST_CASE("parse: identifier without '::' is an expression statement", "[parser]") {
    CHECK(parseDump("a*5;") == "(unit (expr-stmt (binary * (id a) (int 5))))");
}

TEST_CASE("parse: statement starting with an operator throws ParseError", "[parser]") {
    Tokenizer t;
    t.process("*5;");
    SimpleParser p(t);
    REQUIRE_THROWS_AS(p.buildAST(), ParseError);
}

TEST_CASE("parse: missing ';' throws ParseError", "[parser]") {
    Tokenizer t;
    t.process("a::Int");
    SimpleParser p(t);
    REQUIRE_THROWS_AS(p.buildAST(), ParseError);
}

TEST_CASE("parse: missing type identifier throws ParseError", "[parser]") {
    Tokenizer t;
    t.process("a::;");
    SimpleParser p(t);
    REQUIRE_THROWS_AS(p.buildAST(), ParseError);
}
