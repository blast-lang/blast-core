#include <core/lexer/Lexer.hpp>
#include <core/parser/Parser.hpp>
#include <core/context/Scope.hpp>
#include <core/context/ASTContext.hpp>
#include <core/context/Resolver.hpp>
#include <core/ir/IR.hpp>
#include <core/codegen/X86.hpp>
#include <core/utils/Dump.hpp>
#include <core/Exception.hpp>
#include <cstdio>
#include <cstdlib>
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
        std::printf("%s", blast::core::utils::dump(tokenizer).c_str());

        parser.buildAST();
    } catch (const blast::core::LexError& e) {
        std::fprintf(stderr, "blastc: lex error at %zu: %s\n", e.position(), e.what());
        return 1;
    } catch (const blast::core::ParseError& e) {
        std::fprintf(stderr, "blastc: parse error at token %zu: %s\n", e.position(), e.what());
        return 1;
    }

    std::puts("--- AST ---");
    std::printf("%s", blast::core::utils::dumpTree(parser.root()).c_str());

    ASTContext ctx;
    ScopeResolver sr(ctx);
    TypeResolver tr(ctx);

    try {
        // Resolve scopes
        sr.run(*parser.root());
        // Resolve Types
        tr.run(*parser.root());
    } catch (const blast::core::SemanticError& e) {
        std::fprintf(stderr, "blastc: %s\n", e.what());
        return 1;
    }

    
    try {
        blast::core::ir::SSAIR lowerer(ctx);
        const auto& module = lowerer.run(*parser.root());
        std::puts("--- IR ---");
        std::printf("%s", blast::core::utils::dump(module).c_str());

        blast::core::codegen::X86 x86;
        x86.lower(module);
        std::puts("--- X86 ---");
        std::printf("%s", blast::core::utils::dump(x86).c_str());

        x86.emit();

        std::string stem(argv[1]);
        const std::size_t slash = stem.find_last_of('/');
        if (slash != std::string::npos) {
            stem = stem.substr(slash + 1);
        }
        const std::size_t dot = stem.find_last_of('.');
        if (dot != std::string::npos) {
            stem = stem.substr(0, dot);
        }

        const std::string asm_path = stem + ".s";
        std::ofstream asm_file(asm_path);
        if (!asm_file) {
            std::fprintf(stderr, "blastc: cannot write '%s'\n", asm_path.c_str());
            return 1;
        }
        asm_file << x86.out();
        asm_file.close();

        // cc drives as and ld, and brings in the crt startup files and libc
        const std::string cmd = "cc -no-pie " + asm_path + " -o " + stem;
        std::printf("blastc: $ %s\n", cmd.c_str());
        std::fflush(stdout);
        if (std::system(cmd.c_str()) != 0) {
            std::fprintf(stderr, "blastc: assembling and linking '%s' failed\n", asm_path.c_str());
            return 1;
        }
        std::printf("blastc: wrote %s and %s\n", asm_path.c_str(), stem.c_str());

        std::puts("--- RUN ---");
        const std::string run = "./" + stem;
        std::printf("blastc: $ %s\n", run.c_str());
        std::fflush(stdout);
        const int status = std::system(run.c_str());
        std::printf("blastc: %s exited with %d\n", stem.c_str(), status);
    } catch (const blast::core::CodegenError& e) {
        std::fprintf(stderr, "blastc: %s\n", e.what());
        return 1;
    }

    return 0;
}
