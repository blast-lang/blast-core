#include <catch2/catch_test_macros.hpp>
#include <core/context/Scope.hpp>

using namespace blast::core::context;

TEST_CASE("declare: binds a name and returns it", "[scope]") {
    Scope module(Scope::Kind::Module);
    Symbol* s = module.declare(Symbol::Kind::Variable, "a");

    REQUIRE(s != nullptr);
    CHECK(s->name() == "a");
    CHECK(s->kind() == Symbol::Kind::Variable);
    CHECK(s->scope() == &module);
    CHECK(module.lookupLocal("a") == s);
}

TEST_CASE("declare: redeclaring in the same scope fails", "[scope]") {
    Scope module(Scope::Kind::Module);
    REQUIRE(module.declare(Symbol::Kind::Variable, "a") != nullptr);
    CHECK(module.declare(Symbol::Kind::Variable, "a") == nullptr);
}

TEST_CASE("declare: shadowing an outer scope succeeds", "[scope]") {
    Scope module(Scope::Kind::Module);
    Symbol* outer = module.declare(Symbol::Kind::Variable, "a");
    Scope* block = module.createChild(Scope::Kind::Block);
    Symbol* inner = block->declare(Symbol::Kind::Variable, "a");

    REQUIRE(inner != nullptr);
    CHECK(inner != outer);
    CHECK(block->lookup("a").symbol == inner);
    CHECK(module.lookup("a").symbol == outer);
}

TEST_CASE("lookup: walks outward through blocks", "[scope]") {
    Scope module(Scope::Kind::Module);
    Symbol* a = module.declare(Symbol::Kind::Variable, "a");
    Scope* inner = module.createChild(Scope::Kind::Block)->createChild(Scope::Kind::Block);

    CHECK(inner->lookup("a").symbol == a);
    CHECK(inner->lookupLocal("a") == nullptr);
}

TEST_CASE("lookup: reports a miss without claiming ambiguity", "[scope]") {
    Scope module(Scope::Kind::Module);
    const Scope::LookupResult r = module.lookup("nope");

    CHECK(r.symbol == nullptr);
    CHECK_FALSE(r.ambiguous);
    CHECK_FALSE(static_cast<bool>(r));
}

TEST_CASE("lookup: a module is a lookup root", "[scope]") {
    Scope core(Scope::Kind::Module);
    Scope outer(Scope::Kind::Module);
    Symbol* hidden = outer.declare(Symbol::Kind::Variable, "hidden");

    Symbol* inner_sym = outer.createModule("Inner", &core);
    REQUIRE(inner_sym != nullptr);
    Scope* inner = inner_sym->moduleScope();
    REQUIRE(inner != nullptr);

    // Visible from a block of the enclosing module, invisible across the boundary.
    CHECK(outer.createChild(Scope::Kind::Block)->lookup("hidden").symbol == hidden);
    CHECK(inner->lookup("hidden").symbol == nullptr);
}

TEST_CASE("createModule: gives the new module an implicit using Core", "[scope]") {
    Scope core(Scope::Kind::Module);
    Symbol* int_sym = core.declare(Symbol::Kind::Type, "Int");

    Scope root(Scope::Kind::Module);
    Scope* inner = root.createModule("Inner", &core)->moduleScope();

    // Reaches a builtin without naming Core, and from a nested block too.
    CHECK(inner->lookup("Int").symbol == int_sym);
    CHECK(inner->createChild(Scope::Kind::Block)->lookup("Int").symbol == int_sym);
}

TEST_CASE("createModule: refuses a name already taken", "[scope]") {
    Scope core(Scope::Kind::Module);
    Scope root(Scope::Kind::Module);
    REQUIRE(root.createModule("Inner", &core) != nullptr);
    CHECK(root.createModule("Inner", &core) == nullptr);
}

TEST_CASE("lookup: a local declaration shadows a glob import", "[scope]") {
    Scope core(Scope::Kind::Module);
    core.declare(Symbol::Kind::Type, "Int");

    Scope module(Scope::Kind::Module);
    module.addGlobImport(&core);
    Symbol* own = module.declare(Symbol::Kind::Type, "Int");

    REQUIRE(own != nullptr);
    CHECK(module.lookup("Int").symbol == own);
    CHECK_FALSE(module.lookup("Int").ambiguous);
}

TEST_CASE("lookup: two globs supplying one name is ambiguous", "[scope]") {
    Scope a(Scope::Kind::Module);
    Scope b(Scope::Kind::Module);
    a.declare(Symbol::Kind::Function, "f");
    b.declare(Symbol::Kind::Function, "f");

    Scope module(Scope::Kind::Module);
    module.addGlobImport(&a);
    module.addGlobImport(&b);

    const Scope::LookupResult r = module.lookup("f");
    CHECK(r.symbol == nullptr);
    CHECK(r.ambiguous);
}

TEST_CASE("lookup: the same symbol via two globs is not ambiguous", "[scope]") {
    Scope core(Scope::Kind::Module);
    Symbol* f = core.declare(Symbol::Kind::Function, "f");

    Scope module(Scope::Kind::Module);
    module.addGlobImport(&core);
    module.addGlobImport(&core);

    const Scope::LookupResult r = module.lookup("f");
    CHECK(r.symbol == f);
    CHECK_FALSE(r.ambiguous);
}

TEST_CASE("glob import is not transitive", "[scope]") {
    Scope core(Scope::Kind::Module);
    Symbol* i = core.declare(Symbol::Kind::Type, "Int");

    Scope middle(Scope::Kind::Module);
    middle.addGlobImport(&core);

    Scope user(Scope::Kind::Module);
    user.addGlobImport(&middle);

    CHECK(middle.lookup("Int").symbol == i);
    CHECK(user.lookup("Int").symbol == nullptr);
}

TEST_CASE("moduleScope: qualified lookup crosses from a name to a scope", "[scope]") {
    Scope core(Scope::Kind::Module);
    Scope root(Scope::Kind::Module);

    Symbol* mod = root.createModule("Inner", &core);
    Symbol* x = mod->moduleScope()->declare(Symbol::Kind::Variable, "x");

    CHECK(mod->kind() == Symbol::Kind::Module);
    CHECK(root.lookup("Inner").symbol->moduleScope()->lookupLocal("x") == x);
    // Unqualified, from outside, it is invisible.
    CHECK(root.lookup("x").symbol == nullptr);
}

TEST_CASE("symbols: declaration order is preserved", "[scope]") {
    Scope module(Scope::Kind::Module);
    Symbol* a = module.declare(Symbol::Kind::Variable, "a");
    Symbol* b = module.declare(Symbol::Kind::Variable, "b");
    Symbol* c = module.declare(Symbol::Kind::Variable, "c");

    REQUIRE(module.symbols().size() == 3);
    CHECK(module.symbols()[0] == a);
    CHECK(module.symbols()[1] == b);
    CHECK(module.symbols()[2] == c);
}

TEST_CASE("Symbol: id is unset until the resolver hands one out", "[scope]") {
    Scope module(Scope::Kind::Module);
    Symbol* s = module.declare(Symbol::Kind::Variable, "a");

    CHECK_FALSE(s->hasId());
    CHECK(s->id() == INVALID_SYMBOL_ID);
    s->setId(3);
    CHECK(s->hasId());
    CHECK(s->id() == 3);
}
