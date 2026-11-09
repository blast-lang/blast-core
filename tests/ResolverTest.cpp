#include <catch2/catch_test_macros.hpp>
#include <core/parser/Parser.hpp>
#include <core/context/ASTContext.hpp>
#include <core/context/Resolver.hpp>
#include <core/context/Exception.hpp>

using namespace blast::core;
using namespace blast::core::context;
using blast::core::lexer::Tokenizer;

TEST_CASE("resolve: a = 1000 without a declaration is undeclared", "[resolver]") {
    Tokenizer tokenizer;
    tokenizer.process("a = 1000;");
    parser::SimpleParser parser(tokenizer);
    parser.buildAST();

    ASTContext ctx;
    ScopeResolver sr(ctx);
    REQUIRE_THROWS_AS(sr.run(*parser.root()), UndeclaredIdentifier);
}
