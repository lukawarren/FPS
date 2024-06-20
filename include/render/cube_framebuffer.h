#pragma once
#include "pch.h"
#include "render/texture.h"

class CubeFramebuffer
{
public:
    CubeFramebuffer();
    CubeFramebuffer(const CubeFramebuffer&) = delete;
    ~CubeFramebuffer();

    void bind() const;
    void unbind() const;

    std::array<glm::mat4, 6> get_matrices(
        const glm::vec3& centre,
        const float far_plane
    ) const;

    Texture cubemap;

private:
    unsigned int framebuffer;
};