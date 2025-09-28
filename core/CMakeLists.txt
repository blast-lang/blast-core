add_library(
    blastcore SHARED
    blastcore.cpp
)
set_target_properties(blastcore PROPERTIES 
    INSTALL_RPATH_USE_LINK_PATH TRUE
    BUILD_WITH_INSTALL_RPATH TRUE
    # Set RPATH with relative path
    # For example if 'core' is in 'blast/lib', then 'blastld' will be found in 'blast/lib/ld' = './blastld'
    INSTALL_RPATH "\$ORIGIN:\$ORIGIN/blastld"
)

target_link_libraries(blastcore blastld)

install(
    TARGETS blastcore
    EXPORT blast::blastcore
    LIBRARY DESTINATION ${blast_install_libdir}
    RUNTIME DESTINATION ${blast_install_bindir}
    PUBLIC_HEADER DESTINATION ${blast_install_includedir}
)

list(APPEND CMAKE_INSTALL_RPATH ${blast_install_libdir})