#include <catch2/catch_test_macros.hpp>
#include <core/Exception.hpp>
#include <core/context/AstContext.hpp>
#include <core/context/Resolver.hpp>
#include <core/parser/Parser.hpp>

using namespace blast::core;
using namespace blast::core::context;
using blast::core::lexer::Tokenizer;
using blast::core::parser::SimpleParser;

namespace {

// Holds the tokenizer and parser alive: the AST outlives neither.
struct Resolved {
    Tokenizer tokenizer;
    SimpleParser parser{tokenizer};
    AstContext ctx;

    explicit Resolved(const std::string& source) {
        this->tokenizer.process(source);
        this->parser.buildAST();
    }

    void resolveScopes() {
        ScopeResolver sr(this->ctx);
        sr.run(*this->parser.root());
    }

    void resolveTypes() {
        TypeResolver tr(this->ctx);
        tr.run(*this->parser.root());
    }

    void resolveAll() {
        this->resolveScopes();
        this->resolveTypes();
    }
};

} // namespace

TEST_CASE("AstContext: Core declares Int and binds it to a type", "[resolver]") {
    AstContext ctx;
    Symbol* s = ctx.coreScope().lookupLocal("Int");

    REQUIRE(s != nullptr);
    CHECK(s->kind() == Symbol::Kind::Type);
    REQUIRE(s->type() != nullptr);
    CHECK(s->type()->name() == "Int");
    CHECK(s->type()->kind() == Type::Kind::Primitive);
}

TEST_CASE("AstContext: main sees Core through the implicit glob import", "[resolver]") {
    AstContext ctx;
    CHECK(ctx.mainScope().lookup("Int").symbol == ctx.coreScope().lookupLocal("Int"));
    CHECK(ctx.mainScope().lookupLocal("Int") == nullptr);
}

TEST_CASE("AstContext: side tables read back null for an unwritten node", "[resolver]") {
    AstContext ctx;
    CHECK(ctx.nodeSymbol(0) == nullptr);
    CHECK(ctx.nodeType(0) == nullptr);
    CHECK(ctx.nodeSymbol(blast::core::parser::INVALID_NODE_ID) == nullptr);
}

TEST_CASE("AstContext: an unnumbered node is ignored on write", "[resolver]") {
    AstContext ctx;
    Symbol* s = ctx.coreScope().lookupLocal("Int");
    ctx.setNodeSymbol(blast::core::parser::INVALID_NODE_ID, s);
    CHECK(ctx.nodeSymbol(blast::core::parser::INVALID_NODE_ID) == nullptr);
}

TEST_CASE("ScopeResolver: declares a variable in the main scope", "[resolver]") {
    Resolved r("a::Int = 8;");
    r.resolveScopes();

    Symbol* a = r.ctx.mainScope().lookupLocal("a");
    REQUIRE(a != nullptr);
    CHECK(a->kind() == Symbol::Kind::Variable);
    CHECK(a->decl() != nullptr);
}

TEST_CASE("ScopeResolver: records the declaration node in the side table", "[resolver]") {
    Resolved r("a::Int = 8;");
    r.resolveScopes();

    const auto& stmts = r.parser.root()->stmts();
    REQUIRE(stmts.size() == 1);
    const auto* stmt = static_cast<const blast::core::parser::ExprStmt*>(stmts[0].get());
    const auto* decl = static_cast<const blast::core::parser::VarDecl*>(stmt->expr());

    CHECK(r.ctx.nodeSymbol(decl->id()) == r.ctx.mainScope().lookupLocal("a"));
    // The annotation resolves to Core.Int, through its own node.
    CHECK(r.ctx.nodeSymbol(decl->type()->id()) == r.ctx.coreScope().lookupLocal("Int"));
}

TEST_CASE("ScopeResolver: rejects a redeclaration", "[resolver]") {
    Resolved r("a::Int = 1;\na::Int = 2;");
    CHECK_THROWS_AS(r.resolveScopes(), SemanticError);
}

TEST_CASE("ScopeResolver: rejects an unknown type name", "[resolver]") {
    Resolved r("a::Foo = 1;");
    CHECK_THROWS_AS(r.resolveScopes(), SemanticError);
}

TEST_CASE("ScopeResolver: rejects a non-type used as a type", "[resolver]") {
    Resolved r("a::Int = 1;\nb::a = 2;");
    CHECK_THROWS_AS(r.resolveScopes(), SemanticError);
}

TEST_CASE("TypeResolver: binds the annotated type to the symbol", "[resolver]") {
    Resolved r("a::Int = 8;");
    r.resolveAll();

    Symbol* a = r.ctx.mainScope().lookupLocal("a");
    REQUIRE(a != nullptr);
    REQUIRE(a->type() != nullptr);
    CHECK(a->type() == r.ctx.coreScope().lookupLocal("Int")->type());
}

TEST_CASE("TypeResolver: writes the node type side table", "[resolver]") {
    Resolved r("a::Int = 8;");
    r.resolveAll();

    const auto& stmts = r.parser.root()->stmts();
    const auto* stmt = static_cast<const blast::core::parser::ExprStmt*>(stmts[0].get());
    const auto* decl = static_cast<const blast::core::parser::VarDecl*>(stmt->expr());

    CHECK(r.ctx.nodeType(decl->id()) == r.ctx.coreScope().lookupLocal("Int")->type());
}

TEST_CASE("TypeResolver: throws when scopes were never resolved", "[resolver]") {
    Resolved r("a::Int = 8;");
    CHECK_THROWS_AS(r.resolveTypes(), SemanticError);
}

TEST_CASE("resolvers: several declarations all resolve to the same Int", "[resolver]") {
    Resolved r("a::Int = 1;\nb::Int = 2;");
    r.resolveAll();

    Symbol* a = r.ctx.mainScope().lookupLocal("a");
    Symbol* b = r.ctx.mainScope().lookupLocal("b");
    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);
    CHECK(a != b);
    // Interning: one Type object, so identity comparison is enough.
    CHECK(a->type() == b->type());
}

TEST_CASE("parser: NodeIds are dense over the parsed tree", "[resolver]") {
    Resolved r("a::Int = 1;\nb::Int = 2;");
    CHECK(r.parser.nodeCount() > 0);
    CHECK(r.parser.root()->id() == r.parser.nodeCount() - 1);
}
