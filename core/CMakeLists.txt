add_library(
    core SHARED
    core.hpp core.cpp
)

# For example if 'core' is in 'dyload/lib', then 'dyload' will be found in 'dyload/lib/ld' = './dyload'
#INSTALL_RPATH "\$ORIGIN:\$ORIGIN/dyload"
set(CORE_INSTALL_RPATH "\$ORIGIN")
foreach(submod ${BLAST_CORE_MODULES})
    string(APPEND CORE_INSTALL_RPATH ":\$ORIGIN/${submod}")
endforeach()

set_target_properties(core PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
    BUILD_WITH_INSTALL_RPATH TRUE
    # Set RPATH with relative path
    INSTALL_RPATH ${CORE_INSTALL_RPATH}
    # Set lib version
    VERSION ${BLAST_VERSION}
    # Register 'core.hpp' as this module's header
    PUBLIC_HEADER core.hpp
)

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
            INSTALL_RPATH ${CORE_INSTALL_RPATH}
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


#target_link_libraries(core PUBLIC ${BLAST_CORE_MODULES} PRIVATE ${LLVM_DEPS})
target_link_libraries(core PUBLIC ${BLAST_CORE_MODULES} ${LLVM_DEPS})

install(
    TARGETS core
    EXPORT blast::core
    LIBRARY DESTINATION ${blast_install_libdir}
    RUNTIME DESTINATION ${blast_install_bindir}
    PUBLIC_HEADER DESTINATION ${blast_install_includedir}
)

list(APPEND CMAKE_INSTALL_RPATH ${blast_install_libdir})