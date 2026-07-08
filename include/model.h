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
        WEAPON_5 = 0,
        DOOR
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

    static inline constexpr std::array<const char*, 2> MODEL_NAMES =
    {
        "weapon9",
        "door"
    };

    static std::pair<Mesh*, Texture*> load(
        ID id,
        SDL_GPUDevice* device,
        SDL_GPUCopyPass* copy_pass
    );

    Transform transform;
    ID id;
};
