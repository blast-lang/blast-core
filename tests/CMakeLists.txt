include(Catch)

add_executable(blast_tests AutomataTest.cpp)

# Automata is header-only template code — include path only, no DLL dependency
target_include_directories(blast_tests PRIVATE
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}>
)

target_link_libraries(blast_tests PRIVATE Catch2::Catch2WithMain)

catch_discover_tests(blast_tests
    DISCOVERY_MODE PRE_TEST
)
