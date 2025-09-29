add_library(
    core SHARED
    core.hpp core.cpp
)

# For example if 'core' is in 'dyload/lib', then 'dyload' will be found in 'dyload/lib/ld' = './dyload'
#INSTALL_RPATH "\$ORIGIN:\$ORIGIN/dyload"
set(CORE_INSTALL_RPATH "\$ORIGIN")
foreach(submod ${BLAST_CORE_MODULES})
    string(APPEND CORE_INSTALL_RPATH ":\$ORIGIN/${submod}")
endforeach()

set_target_properties(core PROPERTIES
    INSTALL_RPATH_USE_LINK_PATH TRUE
    BUILD_WITH_INSTALL_RPATH TRUE
    # Set RPATH with relative path
    INSTALL_RPATH ${CORE_INSTALL_RPATH}
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