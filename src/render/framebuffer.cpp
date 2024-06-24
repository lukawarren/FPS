#include "render/framebuffer.h"
#include "config.h"

Framebuffer::Framebuffer(const bool is_cubemap)
{
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    if (is_cubemap)
    {
        texture = new Texture(Texture::Type::Cubemap, shadow_size, shadow_size);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture->handle(), 0);
    }
    else
    {
        texture = new Texture(Texture::Type::Flat, shadow_size, shadow_size);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->handle(), 0);
    }

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, shadow_size, shadow_size);
}

void Framebuffer::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::array<glm::mat4, 6> Framebuffer::get_matrices(
    const glm::vec3& centre,
    const float far_plane
) const
{
    const float aspect = (float)shadow_size / (float)shadow_size;
    const float near = 1.0f;
    const glm::mat4 projection = glm::perspective(glm::radians(90.0f), aspect, near, far_plane);

    std::array<glm::mat4, 6> matrices;
    matrices[0] = projection * glm::lookAt(centre, centre + glm::vec3 {  1.0f,  0.0f,  0.0f }, glm::vec3 { 0.0f, -1.0f,  0.0f });
    matrices[1] = projection * glm::lookAt(centre, centre + glm::vec3 { -1.0f,  0.0f,  0.0f }, glm::vec3 { 0.0f, -1.0f,  0.0f });
    matrices[2] = projection * glm::lookAt(centre, centre + glm::vec3 {  0.0f,  1.0f,  0.0f }, glm::vec3 { 0.0f,  0.0f,  1.0f });
    matrices[3] = projection * glm::lookAt(centre, centre + glm::vec3 {  0.0f, -1.0f,  0.0f }, glm::vec3 { 0.0f,  0.0f, -1.0f });
    matrices[4] = projection * glm::lookAt(centre, centre + glm::vec3 {  0.0f,  0.0f,  1.0f }, glm::vec3 { 0.0f, -1.0f,  0.0f });
    matrices[5] = projection * glm::lookAt(centre, centre + glm::vec3 {  0.0f,  0.0f, -1.0f }, glm::vec3 { 0.0f, -1.0f,  0.0f });

    return matrices;
}

glm::mat4 Framebuffer::get_matrix(
    const glm::vec3& min_bounds,
    const glm::vec3& max_bounds,
    const glm::vec3& light_position
) const
{
    // Centre of world
    const glm::vec3 centre = (min_bounds + max_bounds) * 0.5f;

    // View matrix
    const float distance = 1.0f;
    const glm::mat4 view = glm::lookAt(
        light_position * distance,
        centre,
        glm::vec3(0.0f, 1.0f, 0.0f)
    );

    // Projection matrix
    dbg("this is wrong");
    const float left = min_bounds.x;
    const float right = max_bounds.x;
    const float bottom = min_bounds.y;
    const float top = max_bounds.y;
    const float near_plane = 0.01f;
    const float far_plane = 100.0f;
    const glm::mat4 projection = glm::ortho(left, right, bottom, top, near_plane, far_plane);

    return projection * view;
}

Framebuffer::~Framebuffer()
{
    glDeleteFramebuffers(1, &framebuffer);
    delete texture;
}