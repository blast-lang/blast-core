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
#include <unistd.h>

using namespace blast::core::lexer;
using namespace blast::core::context;

static bool buildRuntime() {
    blast::core::codegen::X86 rt;
    rt.lowerBuiltins();
    blast::core::codegen::RegisterAllocator regalloc(rt);

    std::ofstream rt_file("blastrt.s");
    if (!rt_file) {
        std::fprintf(stderr, "blastc: cannot write 'blastrt.s'\n");
        return false;
    }
    rt_file << blast::core::utils::emit(rt, false);
    rt_file.close();

    const std::string cmd = "cc -shared blastrt.s -o libblastrt.so";
    std::printf("blastc: $ %s\n", cmd.c_str());
    std::fflush(stdout);
    if (std::system(cmd.c_str()) != 0) {
        std::fprintf(stderr, "blastc: building the runtime failed\n");
        return false;
    }
    return true;
}

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
    } catch (blast::core::LexError& e) {
        e.addFrame(std::string("in file '") + argv[1] + "'", e.location());
        std::fprintf(stderr, "blastc: lex error: %s\n", blast::core::describe(e).c_str());
        return 1;
    } catch (blast::core::ParseError& e) {
        const auto& tokens = tokenizer.tokens();
        const blast::core::SourceLocation where = e.location().known() || e.position() >= tokens.size()
            ? e.location() : tokens[e.position()].m_location;
        e.addFrame(std::string("in file '") + argv[1] + "'", where);
        std::fprintf(stderr, "blastc: parse error: %s\n", blast::core::describe(e).c_str());
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
    } catch (blast::core::SemanticError& e) {
        e.addFrame(std::string("in file '") + argv[1] + "'", e.location());
        std::fprintf(stderr, "blastc: %s\n", blast::core::describe(e).c_str());
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

        std::puts("--- REGALLOC ---");
        blast::core::codegen::RegisterAllocator regalloc(x86);
        std::printf("%s", blast::core::utils::dump(x86).c_str());

        std::string stem(argv[1]);
        const std::size_t slash = stem.find_last_of('/');
        if (slash != std::string::npos) {
            stem = stem.substr(slash + 1);
        }
        const std::size_t dot = stem.find_last_of('.');
        if (dot != std::string::npos) {
            stem = stem.substr(0, dot);
        }

        const std::string ir_path = stem + ".ir";
        std::ofstream ir_file(ir_path);
        if (!ir_file) {
            std::fprintf(stderr, "blastc: cannot write '%s'\n", ir_path.c_str());
            return 1;
        }
        ir_file << blast::core::utils::dump(module);
        ir_file.close();

        const std::string asm_path = stem + ".s";
        std::ofstream asm_file(asm_path);
        if (!asm_file) {
            std::fprintf(stderr, "blastc: cannot write '%s'\n", asm_path.c_str());
            return 1;
        }
        asm_file << blast::core::utils::emit(x86);
        asm_file.close();

        // cc drives as and ld, and brings in the crt startup files and libc
        if (!buildRuntime()) {
            return 1;
        }

        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd)) == nullptr) {
            std::fprintf(stderr, "blastc: cannot get the current directory\n");
            return 1;
        }
        const std::string cmd = "cc -no-pie " + asm_path + " -L. -lblastrt -Wl,-rpath," + cwd + " -o " + stem;
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
    } catch (blast::core::IRError& e) {
        e.addFrame(std::string("in file '") + argv[1] + "'", e.location());
        std::fprintf(stderr, "blastc: %s\n", blast::core::describe(e).c_str());
        return 1;
    } catch (const blast::core::CodegenError& e) {
        std::fprintf(stderr, "blastc: %s\n", blast::core::describe(e).c_str());
        return 1;
    }

    return 0;
}
