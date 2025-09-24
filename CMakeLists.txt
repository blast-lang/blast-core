cmake_minimum_required(VERSION 3.10)
include(FetchContent)

set(BLAST_VERSION "0.0.1" CACHE STRING "Blast version")

project(
    blast
    LANGUAGES CXX
    VERSION ${BLAST_VERSION}
)

# Set 'BOOST_ROOT' to help cmake find boost
find_package( 
    Boost 1.89 REQUIRED 
    COMPONENTS filesystem system 
)

# Set installation variables
get_filename_component(blast_install_dir ${CMAKE_INSTALL_PREFIX} ABSOLUTE)
get_filename_component(blast_install_libdir ${blast_install_dir}/lib ABSOLUTE)
get_filename_component(blast_install_bindir ${blast_install_dir}/bin ABSOLUTE)
get_filename_component(blast_install_includedir ${blast_install_dir}/include ABSOLUTE)
get_filename_component(blast_install_docdir ${blast_install_dir}/doc ABSOLUTE)

add_subdirectory(ld)
add_subdirectory(core)
add_subdirectory(blastexe)