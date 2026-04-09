add_library(core SHARED
    src/mem.c
)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(core PRIVATE pthread)

# Tests
include(FetchContent)
FetchContent_Declare(
    unity
    GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(unity)

add_executable(core_test test/test.c)
target_link_libraries(core_test PRIVATE core unity)
target_include_directories(core_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_test(NAME core_test COMMAND core_test)
set_tests_properties(core_test PROPERTIES
    ENVIRONMENT "PATH=$<TARGET_FILE_DIR:core>;$ENV{PATH}"
)
