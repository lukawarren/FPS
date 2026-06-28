#pragma once
#include <cstdint>
#include <cstddef>
#include <exception>
#include <string>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <limits>
#include <unordered_set>
#include <thread>

#include <SDL3/SDL.h>
#include <SDL3_shadercross/SDL_shadercross.h>

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/type_ptr.hpp>

#define module_private public
#include <csg.hpp>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSettings.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#define DBG_MACRO_NO_WARNING
#include <dbg.h>

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

inline std::string SHADER_ROOT = "../res/shaders/";
inline std::string TEXTURE_ROOT = "../res/trenchbroom/textures/";
inline std::string MODEL_ROOT = "../res/trenchbroom/models/";
inline std::string MAP_ROOT = "../res/maps/";
inline std::string SPRITE_ROOT = "../res/sprites/";

struct QualitySettings
{
    u32 inverse_render_scale = 3;
    u32 shadow_map_width = 1024;
    u32 shadow_map_height = 1024;
    u32 bloom_downsamples = 5;
    u32 max_spotlights = 6;
};

constexpr static inline QualitySettings QUALITY_SETTINGS = {};