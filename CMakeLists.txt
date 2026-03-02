cmake_minimum_required(VERSION 4.1)
project(CLion_OpenGL)

set(CMAKE_CXX_STANDARD 20)

include_directories(deps/include)

link_directories(deps/lib)

file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.c")

add_executable(CLion_OpenGL ${SOURCES}
        deps/include/Shader.h
        src/Shader.cpp)

target_link_libraries(CLion_OpenGL glfw3 opengl32 gdi32 user32)


