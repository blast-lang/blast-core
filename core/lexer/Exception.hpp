#pragma once
#include <core/Exception.hpp>

namespace blast::core::lexer {

class UnknownInput: public LexError {
public:
    UnknownInput(char character, std::size_t position, SourceLocation location):
        LexError(std::string("Unknown input '") + character + "' (char=" + std::to_string((int)(unsigned char)character) + ")", position, location),
        m_character(character) {}

    char character() const { return m_character; }

private:
    char m_character;
};

} // namespace blast::core::lexer
