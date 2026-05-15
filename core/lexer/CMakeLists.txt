add_library(lexer STATIC
    Lexer.hpp Lexer.cpp
)

set_target_properties(lexer PROPERTIES
    PUBLIC_HEADER Lexer.hpp
    PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX_CXX}"
    EXPORT_NAME core::lexer
)

target_include_directories(lexer PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${blast_install_includedir}>
)

install(
    TARGETS lexer
    EXPORT blast::core::lexer
    ARCHIVE DESTINATION ${blast_install_libdir}
    PUBLIC_HEADER DESTINATION ${blast_install_includedir}/lexer
)
