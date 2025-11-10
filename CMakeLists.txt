cmake_minimum_required(VERSION 3.10)
include(FetchContent)

set(BLAST_VERSION "0.0.1" CACHE STRING "Blast version")
# Append 'blast' to every lib names
#set(CMAKE_SHARED_LIBRARY_PREFIX_CXX "${CMAKE_LIBRARY_PREFIX}BLAST")
set(CMAKE_SHARED_LIBRARY_PREFIX_CXX "libBLAST")

# Where to find BOOST
set(BLAST_BOOST_ROOT ${BOOST_ROOT} CACHE STRING "Boost install location")
# Where to find LLVM
set(BLAST_LLVM_ROOT "" CACHE STRING "LLVM install location")
# CPP standard
set (CMAKE_CXX_STANDARD 23)
# Linker
set(CMAKE_LINKER "BLAST_LLVM_ROOT/bin/lld")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti")

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

function(blastcore_submodule module)
    add_library(
        ${module} SHARED
        # All arguments after the one defined
        ${ARGN}
    )

    set_target_properties(${module} PROPERTIES 
    # Register '${module}.hpp' as this module's header
        PUBLIC_HEADER ${module}.hpp
    # Set lib version
        VERSION ${BLAST_VERSION}
    )

    install(
        TARGETS ${module}
        EXPORT blast::${module}
        LIBRARY DESTINATION ${blast_install_libdir}/${module}
        RUNTIME DESTINATION ${blast_install_bindir}/${module}
        PUBLIC_HEADER DESTINATION ${blast_install_includedir}/${module}
    )
    list(APPEND CMAKE_INSTALL_RPATH ${blast_install_libdir}/${module})
endfunction(blastcore_submodule)



set(BLAST_CORE_MODULES dyload filesystem)

foreach(submod ${BLAST_CORE_MODULES})
    add_subdirectory(${submod})
endforeach()

add_subdirectory(core)
add_subdirectory(blastexe)