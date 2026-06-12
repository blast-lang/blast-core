#pragma once
#include <stdexcept>
#include <string>
#include <cstddef>

namespace blast::core {

class Exception : public std::runtime_error {
public:
    explicit Exception(const std::string& message): std::runtime_error(message) {}
    explicit Exception(const std::runtime_error& e): std::runtime_error(e) {}
    explicit Exception(const std::exception& e): std::runtime_error(e.what()) {}

    Exception(const Exception&) = default;
    Exception(Exception&&) = default;
};

class LexError : public Exception {
public:
    explicit LexError(const std::string& message, std::size_t position = 0):
        Exception(message), m_position(position) {}

    std::size_t position() const { return m_position; }

private:
    std::size_t m_position;
};

} // namespace blast::core
