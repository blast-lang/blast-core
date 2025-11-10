#include "modules.hpp"

namespace blast::modules {
    void load() {
        dylib::library lib("/home/aferreira/projects/blast_install/lib/filesystem/libBLASTfilesystem.so");
        auto hello = lib.get_function<void(void)>("hello");
        hello();
    }
    
} // namespace blast
