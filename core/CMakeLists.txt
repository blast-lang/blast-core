add_library(
    core SHARED
    core.hpp core.cpp
)
set_target_properties(core PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
    BUILD_WITH_INSTALL_RPATH TRUE
    # Set RPATH with relative path
    # For example if 'core' is in 'dyload/lib', then 'dyload' will be found in 'dyload/lib/ld' = './dyload'
    INSTALL_RPATH "\$ORIGIN:\$ORIGIN/dyload"
    # Set lib version
    VERSION ${BLAST_VERSION}
)

target_link_libraries(core ${BLAST_CORE_MODULES})

install(
    TARGETS core
    EXPORT blast::core
    LIBRARY DESTINATION ${blast_install_libdir}
    RUNTIME DESTINATION ${blast_install_bindir}
    PUBLIC_HEADER DESTINATION ${blast_install_includedir}
)

list(APPEND CMAKE_INSTALL_RPATH ${blast_install_libdir})