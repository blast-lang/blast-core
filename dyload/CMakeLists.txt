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

# Build dylib with 'fPIC'
set_property(TARGET dylib PROPERTY POSITION_INDEPENDENT_CODE ON)

blastcore_submodule(
  dyload
  dyload.hpp dyload.cpp
)

target_link_libraries(dyload dylib)