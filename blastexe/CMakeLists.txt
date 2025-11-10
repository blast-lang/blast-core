
# Install CLI11 as header only lib.
file(
    DOWNLOAD https://github.com/CLIUtils/CLI11/releases/download/v2.5.0/CLI11.hpp ${CMAKE_BINARY_DIR}/CLI11.hpp
)
add_library(CLI11 INTERFACE)
set_target_properties(CLI11 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR})

add_executable(
    blast blast.cpp
)

# For example if 'blast' is in 'blast/bin', then 'core' will be found in 'blast/lib/' = '../lib/'
# Same for 'core' link-time dependencies
set(BLASTEXE_INSTALL_RPATH "\$ORIGIN:\$ORIGIN/../lib")
foreach(submod ${BLAST_CORE_MODULES})
    string(APPEND BLASTEXE_INSTALL_RPATH ":\$ORIGIN/../lib/${submod}")
endforeach()

set_target_properties(blast PROPERTIES 
    INSTALL_RPATH_USE_LINK_PATH TRUE
    BUILD_WITH_INSTALL_RPATH TRUE
    # Set RPATH with relative path
    INSTALL_RPATH ${BLASTEXE_INSTALL_RPATH}
)

target_include_directories(blast PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
  $<INSTALL_INTERFACE:${blast_install_includedir}>
)

set(CMAKE_INSTALL_MODE SYMLINK_OR_COPY)

# Install LLVM libs as 'imported' dependancies
function(get_llvm_lib lib)
    add_library(${lib} SHARED IMPORTED)
    # Compute the absolute path to an existing file or directory with symlinks resolved
    # libLLVM.so -> libLLVM.so.21.1 will link and install libLLVM.so.21.1
    file(REAL_PATH ${BLAST_LLVM_ROOT}/lib/lib${lib}.so lib_realpath)
    message("lib${lib}.so -> ${lib_realpath}")
    if(UNIX)
        set_target_properties(
            ${lib} PROPERTIES 
            IMPORTED_LOCATION ${lib_realpath}
            INTERFACE_INCLUDE_DIRECTORIES ${BLAST_LLVM_ROOT}/include
            # Set RPATH with relative path
            INSTALL_RPATH ${BLASTEXE_INSTALL_RPATH}
        )
        # Install imported library into blast's lib dir
        install(
            FILES ${lib_realpath}
            DESTINATION ${blast_install_libdir}
        )
    else()
        message(FATAL "Blast not yet available on Windows")
    endif(UNIX)
endfunction(get_llvm_lib)

set(LLVM_LINK_COMPONENTS support)

# LLVM libs
set(LLVM_DEPS 
    #LLVMCore
    #LLVMSupport
    #LLVMDemangle
    #clangAST
    #clangASTMatchers
    #clangBasic
    #clangFrontend
    #clangSerialization
    #clangTooling

    # Shared libs
    clang
    clang-cpp
    LLVM
)

foreach(lib ${LLVM_DEPS})
    get_llvm_lib(${lib})
endforeach()

target_link_libraries(blast PUBLIC core PRIVATE CLI11 ${LLVM_DEPS})

install(
    TARGETS blast
    EXPORT blast::blast
    LIBRARY DESTINATION ${blast_install_libdir}
    RUNTIME DESTINATION ${blast_install_bindir}
    PUBLIC_HEADER DESTINATION ${blast_install_includedir}
)