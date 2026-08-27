add_library(codegen STATIC
    IRVisitor.hpp IRVisitor.cpp
)

set_target_properties(codegen PROPERTIES
    PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX_CXX}"
    EXPORT_NAME core::codegen
)

target_include_directories(codegen PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${blast_install_includedir}>
)

# IRVisitor.hpp walks ir's types, so the dependency is part of the interface.
target_link_libraries(codegen PUBLIC ir)

install(
    TARGETS codegen
    EXPORT blast::core::codegen
    ARCHIVE DESTINATION ${blast_install_libdir}
)
