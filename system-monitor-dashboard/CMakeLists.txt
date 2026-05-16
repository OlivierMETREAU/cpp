cmake_minimum_required(VERSION 3.16)

project(system-monitor-dashboard VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.11.1)

qt_add_executable(system-monitor-dashboard
    app/main.cpp)

qt_add_qml_module(system-monitor-dashboard
    URI qml
    VERSION 1.0
    QML_FILES
        qml/main.qml
)

target_link_libraries(system-monitor-dashboard PRIVATE Qt6::Quick)

include(GNUInstallDirs)

install(TARGETS system-monitor-dashboard
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

