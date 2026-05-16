# CMakeLists.txt — apply to all targets
add_compile_options(
    -Wall -Wextra -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wold-style-cast
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
)

# Debug build only
target_compile_options(my_target PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined -fno-omit-frame-pointer>
)
target_link_options(my_target PRIVATE
    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
)
