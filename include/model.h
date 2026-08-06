#pragma once
#include "common.h"
#include "transform.h"
#include "render/mesh.h"
#include "render/texture.h"

class Model
{
public:
    enum class ID
    {
        WEAPON_1 = 0,
        WEAPON_2,
        WEAPON_3,
        WEAPON_4,
        WEAPON_5,
        WEAPON_6,
        WEAPON_7,
        WEAPON_8,
        WEAPON_9,
        DOOR,
        BIG_DOOR
    };

    Model(
        const ID id,
        const glm::vec3 position = {},
        const glm::vec3 scale = glm::vec3(1.0f)
    ) : id(id)
    {
        transform.scale = scale;
        transform.position = position;
    }

    static inline constexpr std::array<const char*, 11> MODEL_NAMES =
    {
        "weapon1",
        "weapon2",
        "weapon3",
        "weapon4",
        "weapon5",
        "weapon6",
        "weapon7",
        "weapon8",
        "weapon9",
        "door",
        "big_door"
    };

    static inline constexpr size_t WEAPON_COUNT = 9;

    static std::pair<Mesh*, Texture*> load(
        ID id,
        SDL_GPUDevice* device,
        SDL_GPUCopyPass* copy_pass
    );

    Transform transform;
    ID id;
};
