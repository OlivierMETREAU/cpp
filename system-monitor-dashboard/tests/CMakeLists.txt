find_package(Catch2 3 REQUIRED)

add_executable(core_tests
    test_example.cpp
)
target_link_libraries(core_tests
    PRIVATE Catch2::Catch2WithMain myproject_core
)
find_path(CATCH_INCLUDE_DIR NAMES catch.hpp PATH_SUFFIXES catch2)
target_include_directories(core_tests PRIVATE ${CATCH_INCLUDE_DIR})
include(Catch)
catch_discover_tests(core_tests)
