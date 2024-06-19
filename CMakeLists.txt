cmake_minimum_required(VERSION 3.10)

# Set the project name and version
set(TARGET_NAME FPS)
project(${TARGET_NAME} VERSION 1.0)

# C++ version
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Dependencies
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)

# Add glad (separate target to not break PCH)
add_library(glad STATIC lib/glad/src/glad.c)
target_include_directories(glad PUBLIC "${PROJECT_SOURCE_DIR}/lib/glad/include")
set_source_files_properties(lib/glad/src/glad.c PROPERTIES COMPILE_FLAGS "-w")

# Add files
file(GLOB SOURCES
    src/*.cpp
    src/*/*.cpp
    lib/imgui/*.cpp
    lib/imgui/backends/imgui_impl_opengl3.cpp
    lib/imgui/backends/imgui_impl_glfw.cpp
)

# For Visual Studio, add header and shader files
if(MSVC)
    file(GLOB HEADERS include/*.h)
    file(GLOB SHADERS res/*.vert res/*.frag res/*.geom)
    source_group("Headers" FILES ${HEADERS})
    source_group("Shaders" FILES ${SHADERS})
endif()

# Add project
add_executable(${TARGET_NAME} ${SOURCES} ${HEADERS} ${SHADERS})
target_include_directories(${TARGET_NAME} PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/lib/glad/include"
    "${PROJECT_SOURCE_DIR}/lib/imgui"
    "${PROJECT_SOURCE_DIR}/lib/imgui/backends"
    "${PROJECT_SOURCE_DIR}/lib/dbg-macro/"
)
target_precompile_headers(${TARGET_NAME} PRIVATE
    "${PROJECT_SOURCE_DIR}/include/pch.h"
)

# C++ warnings and compiler flags
if(MSVC)
    target_compile_options(${TARGET_NAME} PUBLIC /W4 /Wv:18)
else()
    target_compile_options(${TARGET_NAME} PRIVATE -O0 -g -Wall -Wextra -pedantic -fdiagnostics-color=always)
endif()

# Dependencies
target_link_libraries(${TARGET_NAME} glad glfw glm)