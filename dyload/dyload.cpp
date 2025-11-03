#include "dyload.hpp"

namespace blast {
    void load() {
        dyload::library lib("/home/aferreira/projects/blast_install/lib/filesystem/libBLASTfilesystem.so");
        auto hello = lib.get_function<void(void)>("hello");
        hello();
    }
    
} // namespace blast
