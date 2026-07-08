#pragma once
#include "common.h"
#include "render/mesh.h"
#include "render/texture.h"

class Model
{
public:
    Model(const std::string& filename, SDL_GPUDevice* device, SDL_GPUCopyPass* copy_pass);
    ~Model();

    Mesh* mesh;
    Texture* texture;

    enum class ID
    {
        WEAPON_5 = 0
    };

    static inline constexpr std::array<const char*, 1> MODEL_NAMES =
    {
        "weapon5"
    };
};
