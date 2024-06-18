#pragma once
#include "pch.h"

class Texture
{
public:
    Texture(
        const std::string& filename,
        const bool use_nearest_filtering = true
    );
    Texture(const Texture&) = delete;
    ~Texture();

    void set_max_mipmap_level(const int max_mipmap_level) const;
    void bind(const unsigned int unit = 0) const;
    void unbind() const;

private:
    unsigned int texture_id;
    int width;
    int height;
};