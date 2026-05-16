cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE ~/Projects/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE FILEPATH "~/Projects/vcpkg/scripts/buildsystems/vcpkg.cmake")

project(system-monitor-dashboard VERSION 0.1 LANGUAGES CXX)

option(BUILD_TESTING "Build tests" OFF)
if(BUILD_TESTING)
  list(APPEND VCPKG_MANIFEST_FEATURES "tests")
endif()

add_compile_options(
    -Wall -Wextra -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
)


set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.11.1)

qt_add_executable(system-monitor-dashboard
    src/main.cpp)

qt_add_qml_module(system-monitor-dashboard
    URI qml
    VERSION 1.0
    QML_FILES
        qml/main.qml
)

target_link_libraries(system-monitor-dashboard PRIVATE Qt6::Quick)

# Debug build only
target_compile_options(system-monitor-dashboard PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined -fno-omit-frame-pointer>
)
target_link_options(system-monitor-dashboard PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)
include(GNUInstallDirs)

install(TARGETS system-monitor-dashboard
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

