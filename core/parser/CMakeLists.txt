add_library(parser STATIC
    Ast.hpp AstVisitor.hpp
    Parser.cpp
)

set_target_properties(parser PROPERTIES
    PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX_CXX}"
    EXPORT_NAME core::parser
)

target_include_directories(parser PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${blast_install_includedir}>
)

install(
    TARGETS parser
    EXPORT blast::core::parser
    ARCHIVE DESTINATION ${blast_install_libdir}
)
