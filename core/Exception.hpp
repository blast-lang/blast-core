#pragma once
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace blast::core {

struct SourceLocation {
    std::size_t line = 0;
    std::size_t column = 0;

    bool known() const { return line != 0; }
};

struct Frame {
    std::string label;
    SourceLocation location;
};

class Exception: public std::runtime_error {
public:
    explicit Exception(const std::string& message): std::runtime_error(message) {}
    explicit Exception(const std::runtime_error& e): std::runtime_error(e) {}
    explicit Exception(const std::exception& e): std::runtime_error(e.what()) {}

    Exception(const Exception&) = default;
    Exception(Exception&&) = default;

    Exception& addFrame(std::string label, SourceLocation location = {}) {
        m_frames.push_back({std::move(label), location});
        return *this;
    }

    const std::vector<Frame>& frames() const { return m_frames; }

private:
    std::vector<Frame> m_frames;
};

// Message, frames and locations as a multi-line report for the user.
std::string describe(const Exception& e);

class LexError: public Exception {
public:
    explicit LexError(const std::string& message, std::size_t position = 0, SourceLocation location = {}):
        Exception(message), m_position(position), m_location(location) {}

    std::size_t position() const { return m_position; }
    SourceLocation location() const { return m_location; }

private:
    std::size_t m_position;
    SourceLocation m_location;
};

class ParseError: public Exception {
public:
    // position is the index of the offending token in the token stream.
    explicit ParseError(const std::string& message, std::size_t position = 0, SourceLocation location = {}):
        Exception(message), m_position(position), m_location(location) {}

    std::size_t position() const { return m_position; }
    SourceLocation location() const { return m_location; }

private:
    std::size_t m_position;
    SourceLocation m_location;
};

// Raised while resolving names and types: redeclaration, unknown name,
// ambiguous import, type mismatch.
class SemanticError: public Exception {
public:
    explicit SemanticError(const std::string& message, SourceLocation location = {}):
        Exception(message), m_location(location) {}

    SourceLocation location() const { return m_location; }

private:
    SourceLocation m_location;
};

// Raised while lowering the AST to IR.
class IRError: public Exception {
public:
    explicit IRError(const std::string& message): Exception(message) {}
};

// Raised while lowering the IR to machine code: unknown identifier,
// unsupported node, unsupported type, ...
class CodegenError: public Exception {
public:
    explicit CodegenError(const std::string& message): Exception(message) {}
};

class AutomataError: public Exception {
public:
    AutomataError(const std::string& message, std::size_t state): Exception(message), m_state(state) {}

    std::size_t state() const { return m_state; }

private:
    std::size_t m_state;
};

} // namespace blast::core
