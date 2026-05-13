add_library(core SHARED
    Exception.hpp Exception.cpp
    Automata.hpp
)

set_target_properties(core PROPERTIES
    PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX_CXX}"
    PUBLIC_HEADER Exception.hpp
)

target_include_directories(core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${blast_install_includedir}>
)

install(
    TARGETS core
    EXPORT blast::core
    LIBRARY DESTINATION ${blast_install_libdir}
    RUNTIME DESTINATION ${blast_install_libdir}
    PUBLIC_HEADER DESTINATION ${blast_install_includedir}
)

add_subdirectory(lexer)
add_subdirectory(parser)

add_executable(blastc blastc.cpp)
target_link_libraries(blastc PRIVATE core lexer parser)

install(
    TARGETS blastc
    RUNTIME DESTINATION ${blast_install_bindir}
)
