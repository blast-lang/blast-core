add_library(
    core SHARED
    core.cpp
)
set_target_properties(core PROPERTIES 
    INSTALL_RPATH_USE_LINK_PATH TRUE
    BUILD_WITH_INSTALL_RPATH TRUE
    # Set RPATH with relative path
    # For example if 'core' is in 'blast/lib', then 'ld' will be found in 'blast/lib/ld' = './ld'
    INSTALL_RPATH "\$ORIGIN:\$ORIGIN/ld"
)

target_link_libraries(core ld)

install(
    TARGETS core
    EXPORT blast::core
    LIBRARY DESTINATION ${blast_install_libdir}
    RUNTIME DESTINATION ${blast_install_bindir}
    PUBLIC_HEADER DESTINATION ${blast_install_includedir}
)

list(APPEND CMAKE_INSTALL_RPATH ${blast_install_libdir})