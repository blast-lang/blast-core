#include <core/lexer/Lexer.hpp>
#include <core/parser/Parser.hpp>
#include <core/context/Scope.hpp>
#include <core/context/AstContext.hpp>
#include <core/context/Resolver.hpp>
#include <core/ir/TAC.hpp>
#include <core/Exception.hpp>
#include <cstdio>
#include <fstream>
#include <string>

using namespace blast::core::lexer;
using namespace blast::core::context;
                                     
    

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::puts("usage: blastc <file>");
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file) {
        std::fprintf(stderr, "blastc: cannot open '%s'\n", argv[1]);
        return 1;
    }
    std::string source(std::istreambuf_iterator<char>(file), {});

    blast::core::lexer::Tokenizer tokenizer;
    blast::core::parser::SimpleParser parser(tokenizer);
    try {
        tokenizer.process(source);
        tokenizer.print();

        parser.buildAST();
    } catch (const blast::core::LexError& e) {
        std::fprintf(stderr, "blastc: lex error at %zu: %s\n", e.position(), e.what());
        return 1;
    } catch (const blast::core::ParseError& e) {
        std::fprintf(stderr, "blastc: parse error at token %zu: %s\n", e.position(), e.what());
        return 1;
    }

    std::puts("--- AST ---");
    std::printf("%s", blast::core::parser::dumpTree(parser.root()).c_str());

    AstContext ctx;
    ScopeResolver sr(ctx);
    TypeResolver tr(ctx);

    // Resolve scopes
    sr.run(*parser.root());
    // Resolve Types
    tr.run(*parser.root());

    
    try {
        blast::core::ir::tac::ASTtoTAC lowerer;
        const auto fn = lowerer.run(*parser.root());
        std::puts("--- TAC ---");
        std::printf("%s", blast::core::ir::tac::dump(fn).c_str());
    } catch (const blast::core::CodegenError& e) {
        std::fprintf(stderr, "blastc: %s\n", e.what());
        return 1;
    }

    return 0;
}
