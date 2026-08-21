cmake_minimum_required(VERSION 3.10)
include(FetchContent)

set(BLAST_VERSION "0.0.1" CACHE STRING "Blast version")
# Append 'blast' to every lib names
set(CMAKE_SHARED_LIBRARY_PREFIX_CXX "libBLAST")

# CPP standard
set(CMAKE_CXX_STANDARD 23)

# BLAST discriminates AST/IR nodes with its own kind-enum (isa/dyn_cast style),
# so C++ language-level RTTI (dynamic_cast/typeid) is not needed.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

project(
    blast
    LANGUAGES CXX
    VERSION ${BLAST_VERSION}
)

# Set installation variables
get_filename_component(blast_install_dir ${CMAKE_INSTALL_PREFIX} ABSOLUTE)
get_filename_component(blast_install_libdir ${blast_install_dir}/lib ABSOLUTE)
get_filename_component(blast_install_bindir ${blast_install_dir}/bin ABSOLUTE)
get_filename_component(blast_install_includedir ${blast_install_dir}/include ABSOLUTE)
get_filename_component(blast_install_docdir ${blast_install_dir}/doc ABSOLUTE)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.8.1
)
FetchContent_MakeAvailable(Catch2)

include(CTest)

add_subdirectory(core)
add_subdirectory(tests)