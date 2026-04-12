cmake_minimum_required(VERSION 3.10)
include(FetchContent)

set(BLAST_VERSION "0.0.1" CACHE STRING "Blast version")
# Append 'blast' to every lib names
set(CMAKE_SHARED_LIBRARY_PREFIX_CXX "libBLAST")

# Where to find LLVM
set(BLAST_LLVM_ROOT "" CACHE STRING "LLVM install location")
# CPP standard
set(CMAKE_CXX_STANDARD 23)
# Linker
set(CMAKE_LINKER "BLAST_LLVM_ROOT/bin/lld")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -stdlib=libc++")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -unwindlib=libunwind -rtlib=compiler-rt")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++ -unwindlib=libunwind -rtlib=compiler-rt")

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
add_subdirectory(blastexe)
add_subdirectory(tests)