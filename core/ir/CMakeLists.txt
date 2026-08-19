add_library(ir STATIC
    TAC.hpp TAC.cpp
)

set_target_properties(ir PROPERTIES
    PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX_CXX}"
    EXPORT_NAME core::ir
)

target_include_directories(ir PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${blast_install_includedir}>
)

# TAC.hpp holds context::Type* on values, so the dependency is part of the
# interface, not just the implementation.
target_link_libraries(ir PUBLIC context parser)

install(
    TARGETS ir
    EXPORT blast::core::ir
    ARCHIVE DESTINATION ${blast_install_libdir}
)
