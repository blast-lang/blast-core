
# Install CLI11 as header only lib.
file(
    DOWNLOAD https://github.com/CLIUtils/CLI11/releases/download/v2.5.0/CLI11.hpp ${CMAKE_BINARY_DIR}/CLI11.hpp
)
add_library(CLI11 INTERFACE)
set_target_properties(CLI11 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR})

add_executable(
    blast blast.cpp
)

# For example if 'blast' is in 'blast/bin', then 'core' will be found in 'blast/lib/' = '../lib/'
# Same for 'core' link-time dependencies
set(BLASTEXE_INSTALL_RPATH "\$ORIGIN:\$ORIGIN/../lib")
foreach(submod ${BLAST_CORE_MODULES})
    string(APPEND BLASTEXE_INSTALL_RPATH ":\$ORIGIN/../lib/${submod}")
endforeach()

set_target_properties(blast PROPERTIES 
    INSTALL_RPATH_USE_LINK_PATH TRUE
    BUILD_WITH_INSTALL_RPATH TRUE
    # Set RPATH with relative path
    INSTALL_RPATH ${BLASTEXE_INSTALL_RPATH}
)

target_include_directories(blast PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
  $<INSTALL_INTERFACE:${blast_install_includedir}>
)

target_link_libraries(blast PUBLIC core PRIVATE CLI11)

install(
    TARGETS blast
    EXPORT blast::blast
    LIBRARY DESTINATION ${blast_install_libdir}
    RUNTIME DESTINATION ${blast_install_bindir}
    PUBLIC_HEADER DESTINATION ${blast_install_includedir}
)