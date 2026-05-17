cmake_minimum_required(VERSION 3.28)   # bump this too, 3.16 is old

set(CMAKE_TOOLCHAIN_FILE ~/Projects/vcpkg/scripts/buildsystems/vcpkg.cmake
    CACHE FILEPATH "vcpkg toolchain")

project(system-monitor-dashboard VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(
    -Wall -Wextra -Wpedantic
    -Wshadow -Wnon-virtual-dtor -Wold-style-cast
    -Woverloaded-virtual -Wconversion -Wsign-conversion
)

# Qt
find_package(Qt6 6 REQUIRED COMPONENTS 
    Quick
    QuickControls2    
    Qml)
qt_standard_project_setup(REQUIRES 6.6)

add_subdirectory(src/core)     # ← add this before the executable

qt_add_executable(system-monitor-dashboard src/main.cpp)

qt_add_qml_module(system-monitor-dashboard
    URI qml
    VERSION 1.0
    QML_FILES qml/main.qml
)

target_link_libraries(system-monitor-dashboard PRIVATE
        Qt6::Quick
        Qt6::QuickControls2   # ← add this
        Qt6::Qml              # ← add this
        core_lib)

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

# Tests
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    list(APPEND VCPKG_MANIFEST_FEATURES "tests")
    enable_testing()
    add_subdirectory(tests)
endif()