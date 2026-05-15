#include <core/lexer/Lexer.hpp>
#include <cstdio>
#include <fstream>
#include <string>

using namespace blast::core::lexer;

constexpr std::string to_string(MetaLexer::TokenKind k) noexcept {             
    switch (k) {                                                         
        case MetaLexer::TokenKind::NONE:       return "NONE";                       
        case MetaLexer::TokenKind::IDENTIFIER: return "IDENTIFIER";                 
        case MetaLexer::TokenKind::END_RULE:    return "END_RULE";                    
        case MetaLexer::TokenKind::LEXEM:  return "LEXEM";                  
        case MetaLexer::TokenKind::BIN_OP:   return "BIN_OP";         
        case MetaLexer::TokenKind::UNA_OP:    return "UNA_OP";
        case MetaLexer::TokenKind::CAPTURE:    return "CAPTURE";
        case MetaLexer::TokenKind::RANGE:    return "RANGE";                     
    }                                                                    
    return "UNKNOWN";         
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

    blast::core::lexer::MetaLexer tokenizer;
    auto tokens = tokenizer.process(source);

    std::printf("Tokens: \n");
    for (auto& tok : tokens)
        std::printf("[%s] \t %s\n", to_string(tok.m_kind).c_str(), tok.m_value.c_str());

    return 0;
}
