
# Install CLI11 as header only lib.
file(
    DOWNLOAD https://github.com/CLIUtils/CLI11/releases/download/v2.5.0/CLI11.hpp ${CMAKE_BINARY_DIR}/CLI11.hpp
)
add_library(CLI11 INTERFACE)
set_target_properties(CLI11 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR})

add_executable(
    blast blast.cpp
)

set_target_properties(blast PROPERTIES 
    INSTALL_RPATH_USE_LINK_PATH TRUE
    BUILD_WITH_INSTALL_RPATH TRUE
    # Set RPATH with relative path
    # For example if 'blast' is in 'blast/bin', then 'core' will be found in 'blast/lib/' = '../lib/'
    # Same for 'core' link-time dependencies 
    INSTALL_RPATH "\$ORIGIN:\$ORIGIN/../lib:\$ORIGIN/../lib/ld"
)

target_link_libraries(blast core CLI11)

install(
    TARGETS blast
    EXPORT blast::blast
    LIBRARY DESTINATION ${blast_install_libdir}
    RUNTIME DESTINATION ${blast_install_bindir}
    PUBLIC_HEADER DESTINATION ${blast_install_includedir}
)