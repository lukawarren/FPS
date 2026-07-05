cmake_minimum_required(VERSION 3.24)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(FPS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# reactphysics3d will choose release by default
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Choose the type of build." FORCE)
endif()

# Options for the build
option(USE_SYSTEM_SDL3 "Use installed SDL3 instead of bundled copy" OFF)
option(USE_SYSTEM_SHADERCROSS "Use installed SDL3_shadercross instead of bundled copy" OFF)

# Cache options to configure dependencies before adding their subdirectories
set(SDL_TESTS OFF CACHE INTERNAL "")
set(SDL_EXAMPLES OFF CACHE INTERNAL "")
set(SDLSHADERCROSS_VENDORED ON CACHE INTERNAL "")
set(GLM_BUILD_TESTS OFF CACHE INTERNAL "")
set(JOLT_PHYSICS_BUILD_TESTS OFF CACHE INTERNAL "")
set(JOLT_PHYSICS_BUILD_SAMPLES OFF CACHE INTERNAL "")
set(JOLT_PHYSICS_BUILD_DEBUG_VIEWER OFF CACHE INTERNAL "")
set(JOLT_PHYSICS_BUILD_UNIT_TESTS OFF CACHE INTERNAL "")
set(RECASTNAVIGATION_DEMO OFF CACHE INTERNAL "")
set(RECASTNAVIGATION_TESTS OFF CACHE INTERNAL "")
set(RECASTNAVIGATION_EXAMPLES OFF CACHE INTERNAL "")

# Add submodules
if(USE_SYSTEM_SDL3)
    find_package(SDL3 REQUIRED CONFIG)
else()
    add_subdirectory(lib/SDL)
endif()
if(USE_SYSTEM_SHADERCROSS)
    find_package(SDL3_shadercross REQUIRED CONFIG)
else()
    add_subdirectory(lib/SDL_shadercross)
endif()
add_subdirectory(lib/glm)
add_subdirectory(lib/dbg-macro)
add_subdirectory(lib/JoltPhysics/Build)
add_subdirectory(lib/tinyobjloader)
add_subdirectory(lib/recastnavigation)
add_subdirectory(lib/miniaudio)

# Find shadercross
if(USE_SYSTEM_SHADERCROSS)
    find_program(SHADERCROSS_EXECUTABLE
        NAMES shadercross SDL3_shadercross shadercross-cli
        REQUIRED
    )
else()
    set(SHADERCROSS_EXECUTABLE
        $<TARGET_FILE:shadercross>
    )
endif()

# Source discovery
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    lib/tiny_csg/query*.cpp
    lib/tiny_csg/csg.cpp
    lib/tiny_csg/rebuild.cpp
)
add_executable(${PROJECT_NAME}
    ${SOURCES}
    lib/imgui/imgui.cpp
    lib/imgui/imgui_widgets.cpp
    lib/imgui/imgui_tables.cpp
    lib/imgui/imgui_draw.cpp
    lib/imgui/backends/imgui_impl_sdl3.cpp
    lib/imgui/backends/imgui_impl_sdlgpu3.cpp
)

# Shadercross
file(GLOB VS_SHADERS CONFIGURE_DEPENDS
    shaders/*.vertex.hlsl
)
file(GLOB PS_SHADERS CONFIGURE_DEPENDS
    shaders/*.fragment.hlsl
)
set(COMPILED_SHADER_DIR
    ${CMAKE_CURRENT_BINARY_DIR}/shaders
)
file(MAKE_DIRECTORY ${COMPILED_SHADER_DIR})
set(COMPILED_SHADERS)
foreach(SHADER ${VS_SHADERS} ${PS_SHADERS})
    get_filename_component(NAME ${SHADER} NAME)
    set(OUTPUT
        ${COMPILED_SHADER_DIR}/${NAME}.bin
    )
    add_custom_command(
        OUTPUT ${OUTPUT}
        COMMAND ${SHADERCROSS_EXECUTABLE}
            "${SHADER}"
            -I "${CMAKE_SOURCE_DIR}/shaders"
            -d SPIRV
            -o "${OUTPUT}"
        DEPENDS
            ${SHADER}
        COMMENT "Compiling shader ${NAME}"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${OUTPUT})
endforeach()
add_custom_target(Shaders DEPENDS ${COMPILED_SHADERS})
add_dependencies(${PROJECT_NAME} Shaders)

# Target Configurations
target_include_directories(${PROJECT_NAME} PRIVATE
    include
    lib/stb
    lib/dbg-macro/
    lib/tiny_csg/
    lib/imgui/
    lib/imgui/backends
    lib/
    ${JoltPhysics_SOURCE_DIR}/..
)
target_precompile_headers(${PROJECT_NAME} PRIVATE include/common.h)

target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::SDL3
    SDL3_shadercross::SDL3_shadercross
    glm::glm
    Jolt::Jolt
    tinyobjloader
    RecastNavigation::Recast
    RecastNavigation::Detour
    miniaudio
)

# Fix warning / error with Clang
target_compile_options(${PROJECT_NAME} PRIVATE
    $<$<CXX_COMPILER_ID:Clang>:-Wno-address-of-temporary>
    $<$<CXX_COMPILER_ID:AppleClang>:-Wno-address-of-temporary>
    -fno-rtti # To align with Jolt
)
