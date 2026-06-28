cmake_minimum_required(VERSION 3.24)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(FPS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# reactphysics3d will choose release by default
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

# Cache options to configure dependencies before adding their subdirectories
set(SDL_TESTS OFF CACHE INTERNAL "")
set(SDL_EXAMPLES OFF CACHE INTERNAL "")
set(SDLSHADERCROSS_VENDORED ON CACHE INTERNAL "")
set(GLM_BUILD_TESTS OFF CACHE INTERNAL "")
set(JOLT_PHYSICS_BUILD_TESTS OFF CACHE INTERNAL "")
set(JOLT_PHYSICS_BUILD_SAMPLES OFF CACHE INTERNAL "")
set(JOLT_PHYSICS_BUILD_DEBUG_VIEWER OFF CACHE INTERNAL "")
set(JOLT_PHYSICS_BUILD_UNIT_TESTS OFF CACHE INTERNAL "")

# Add submodules
add_subdirectory(lib/SDL)
add_subdirectory(lib/SDL_shadercross)
add_subdirectory(lib/glm)
add_subdirectory(lib/dbg-macro)
add_subdirectory(lib/JoltPhysics/Build)
add_subdirectory(lib/tinyobjloader)

# Source discovery
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    lib/tiny_csg/query*.cpp
    lib/tiny_csg/csg.cpp
    lib/tiny_csg/rebuild.cpp
)
add_executable(${PROJECT_NAME} ${SOURCES})

# Target Configurations
target_include_directories(${PROJECT_NAME} PRIVATE
    include
    lib/stb
    lib/dbg-macro/
    lib/tiny_csg/
    ${JoltPhysics_SOURCE_DIR}/..
)
target_precompile_headers(${PROJECT_NAME} PRIVATE include/common.h)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    SDL3_shadercross::SDL3_shadercross
    glm::glm
    Jolt::Jolt
    tinyobjloader
)

# Fix warning / error with Clang
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-Wno-address-of-temporary>
    $<$<CXX_COMPILER_ID:AppleClang>:-Wno-address-of-temporary>
)