cmake_minimum_required(VERSION 3.24)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(FPS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Cache options to configure dependencies before adding their subdirectories
set(SDL_TESTS OFF CACHE INTERNAL "")
set(SDL_EXAMPLES OFF CACHE INTERNAL "")
set(SDLSHADERCROSS_VENDORED ON CACHE INTERNAL "")
set(GLM_BUILD_TESTS OFF CACHE INTERNAL "")

# Add submodules from lib/
add_subdirectory(lib/SDL)
add_subdirectory(lib/SDL_shadercross)
add_subdirectory(lib/glm)
add_subdirectory(lib/dbg-macro)

# Source discovery
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    lib/tiny_csg/query*.cpp
    lib/tiny_csg/csg.cpp
    lib/tiny_csg/rebuild.cpp
)
add_executable(${PROJECT_NAME} ${SOURCES})

# Target Configurations
target_include_directories(${PROJECT_NAME} PUBLIC
    include
    lib/stb
    lib/dbg-macro/
    lib/tiny_csg/
)
target_precompile_headers(${PROJECT_NAME} PRIVATE include/common.h)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    SDL3_shadercross::SDL3_shadercross
    glm::glm
)

# Fix warning / error with Clang
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-Wno-address-of-temporary>
    $<$<CXX_COMPILER_ID:AppleClang>:-Wno-address-of-temporary>
)