# Install CLI11 as header-only lib
file(
    DOWNLOAD https://github.com/CLIUtils/CLI11/releases/download/v2.5.0/CLI11.hpp
    ${CMAKE_BINARY_DIR}/CLI11.hpp
)
add_library(CLI11 INTERFACE)
set_target_properties(CLI11 PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_BINARY_DIR})

add_executable(blast blastexe.cpp)

set_target_properties(blast PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_include_directories(blast PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(blast PRIVATE core CLI11)

install(
    TARGETS blast
    RUNTIME DESTINATION ${blast_install_bindir}
)
