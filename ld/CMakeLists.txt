project(balst::ld)

FetchContent_Declare(
    dylib
    GIT_REPOSITORY "https://github.com/martin-olivier/dylib"
    GIT_TAG        "v3.0.1"
    #CONFIGURE_COMMAND ""
    #BUILD_COMMAND ""
    #INSTALL_COMMAND ""
    #EXCLUDE_FROM_ALL -> Only available in Cmake 2.38
)
#FetchContent_MakeAvailable(dylib)

# For Cmake < 3.28, use this trick to NOT install anything from dylib
FetchContent_GetProperties(dylib)
if(NOT dylib_POPULATED)
  FetchContent_Populate(dylib)
  add_subdirectory(${dylib_SOURCE_DIR} ${dylib_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_library(
    ld SHARED
    ld.hpp ld.cpp
)

set_target_properties(ld PROPERTIES 
# Register 'ld.hpp' as this module's header
    PUBLIC_HEADER ld.hpp
# Set lib version
    #VERSION ${BLAST_VERSION}
)

target_link_libraries(ld dylib)

install(
    TARGETS ld
    EXPORT blast::ld
    LIBRARY DESTINATION ${blast_install_libdir}/ld
    RUNTIME DESTINATION ${blast_install_bindir}/ld
    PUBLIC_HEADER DESTINATION ${blast_install_includedir}/ld
)

list(APPEND CMAKE_INSTALL_RPATH ${blast_install_libdir}/ld)