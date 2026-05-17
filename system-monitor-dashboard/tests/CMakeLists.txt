find_package(Catch2 3 REQUIRED)

add_executable(core_tests
    test_systemSnapshot.cpp
)
target_link_libraries(core_tests
    PRIVATE Catch2::Catch2WithMain
    PRIVATE core_lib
)
find_path(CATCH_INCLUDE_DIR NAMES catch.hpp PATH_SUFFIXES catch2)
target_include_directories(core_tests PRIVATE ${CATCH_INCLUDE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/src/core)
include(CTest)
include(Catch)
catch_discover_tests(core_tests)
