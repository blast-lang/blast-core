add_library(utils STATIC
    Dump.hpp Dump.cpp
)

set_target_properties(utils PROPERTIES
    PREFIX "${CMAKE_SHARED_LIBRARY_PREFIX_CXX}"
    EXPORT_NAME core::utils
)

target_include_directories(utils PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${blast_install_includedir}>
)

# Dump.hpp names lexer, parser and ir types in its signatures, so every stage it
# renders is part of the interface. Nothing may depend on utils in return.
target_link_libraries(utils PUBLIC lexer parser ir)
# The IR walk is an implementation detail: no codegen type reaches Dump.hpp.
target_link_libraries(utils PRIVATE codegen)

install(
    TARGETS utils
    EXPORT blast::core::utils
    ARCHIVE DESTINATION ${blast_install_libdir}
)
