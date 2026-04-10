blastcore_submodule(parser
    parser.hpp parser.cpp
)

target_include_directories(parser PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${blast_install_includedir}>
)
