#include <core/lexer/Lexer.hpp>
#include <cstdio>
#include <fstream>
#include <string>

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
    auto tokens = tokenizer.process(source);

    std::printf("Tokens: \n");
    for (auto& tok : tokens)
        std::printf("[%d] '%s'\n", (int)tok.m_kind, tok.m_value.c_str());

    return 0;
}
