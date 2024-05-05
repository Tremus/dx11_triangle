cmake_minimum_required(VERSION 3.16)
project(dx11_triangle)
set(CMAKE_C_STANDARD 99)

add_compile_options(
    -Werror=return-type
    -Werror=shadow
    -Werror=incompatible-pointer-types
    -Werror=parentheses
    -Werror=excess-initializers
    # -Wunused-function
    # -Wunused-variable

    -Wno-deprecated
    -Wno-multichar
    -Wno-nullability-completeness
    -Wno-writable-strings
    -Wno-c2x-extensions
    -Wno-c++14-extensions
    -Wno-c++17-extensions
    -Wno-c++20-extensions
    -Wno-microsoft-enum-forward-reference)

add_executable(main WIN32 main.c)
target_compile_definitions(main PRIVATE SHADER_PATH=L"${PROJECT_SOURCE_DIR}/shaders/")