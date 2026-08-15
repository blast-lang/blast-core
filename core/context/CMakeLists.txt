add_library(context STATIC
    Type.hpp Type.cpp
    SymblTlb.hpp SymblTlb.cpp
    AstContext.hpp AstContext.cpp
)

set_target_properties(context PROPERTIES
    PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX_CXX}"
    EXPORT_NAME core::context
)

target_include_directories(context PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${blast_install_includedir}>
)

install(
    TARGETS context
    EXPORT blast::core::context
    ARCHIVE DESTINATION ${blast_install_libdir}
)
