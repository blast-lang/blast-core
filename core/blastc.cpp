#include <core/lexer/Lexer.hpp>
#include <cstdio>
#include <fstream>
#include <string>

using namespace blast::core::lexer;
                                     
    

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
    tokenizer.process(source);
    tokenizer.print();

    return 0;
}
