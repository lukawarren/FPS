#pragma once
#include "pch.h"
#include "render/texture.h"

class Framebuffer
{
public:
    Framebuffer(const bool is_cubemap = false);
    Framebuffer(const Framebuffer&) = delete;
    ~Framebuffer();

    void bind() const;
    void unbind() const;

    // For point lights
    std::array<glm::mat4, 6> get_matrices(
        const glm::vec3& centre,
        const float far_plane
    ) const;

    // For directional lights
    glm::mat4 get_matrix(
        const glm::vec3& min_bounds,
        const glm::vec3& max_bounds,
        const glm::vec3& light_position
    ) const;

    Texture* texture;

private:
    unsigned int framebuffer;
};