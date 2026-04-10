#pragma once
#include <stdexcept>
#include <string>

namespace blast::core {

class Exception : public std::runtime_error {
public:
    explicit Exception(const std::string& message): std::runtime_error(message) {}
    explicit Exception(const std::runtime_error& e): std::runtime_error(e) {}
    explicit Exception(const std::exception& e): std::runtime_error(e.what()) {}

    Exception(const Exception&) = default;
    Exception(Exception&&) = default;
};

} // namespace blast::core
