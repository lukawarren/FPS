#pragma once
#include "pch.h"

class Texture
{
public:
    // For regular textures
    Texture(
        const std::string& filename,
        const bool use_nearest_filtering = false,
        const bool is_srgb = true
    );

    // For cubemaps
    enum class Type { Cubemap };
    Texture(const Type type, const unsigned int width, const unsigned int height);

    Texture(const Texture&) = delete;
    ~Texture();

    void bind(const unsigned int unit = 0) const;
    void unbind() const;

    unsigned int handle() const { return texture_id; }

private:
    unsigned int texture_id;
    unsigned int texture_format;
};