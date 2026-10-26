#include "Exception.hpp"

namespace blast::core {

std::string describe(const Exception& e) {
    std::string out = e.what();
    for (auto it = e.frames().rbegin(); it != e.frames().rend(); ++it) {
        out += "\n  " + it->label;
        if (it->location.known()) {
            out += " (line " + std::to_string(it->location.line) + ", col " + std::to_string(it->location.column) + ")";
        }
    }
    return out;
}

} // namespace blast::core
