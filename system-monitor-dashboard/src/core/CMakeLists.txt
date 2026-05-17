add_library(core_lib
    systemSnapshot.cpp
    systemMonitor.cpp
)

file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS "*.cpp")  # picks it up automatically

target_include_directories(core_lib
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}   # lets consumers #include "systemSnapshot.hpp"
)

target_compile_features(core_lib PUBLIC cxx_std_23)