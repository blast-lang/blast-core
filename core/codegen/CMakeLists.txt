add_library(codegen STATIC
    X86.hpp X86.cpp
)

set_target_properties(codegen PROPERTIES
    PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX_CXX}"
    EXPORT_NAME core::codegen
)

target_include_directories(codegen PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${blast_install_includedir}>
)

# X86.hpp names ir types in its signatures, so the IR is part of the interface.
target_link_libraries(codegen PUBLIC ir)

install(
    TARGETS codegen
    EXPORT blast::core::codegen
    ARCHIVE DESTINATION ${blast_install_libdir}
)
