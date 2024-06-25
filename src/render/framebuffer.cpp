#include "render/framebuffer.h"
#include "config.h"

Framebuffer::Framebuffer(const bool is_cubemap)
{
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    if (is_cubemap)
    {
        width = height = point_shadow_size;
        texture = new Texture(Texture::Type::Cubemap, width, height);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, texture->handle(), 0);
    }
    else
    {
        width = height = directional_shadow_size;
        texture = new Texture(Texture::Type::Flat, width, height);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture->handle(), 0);
    }

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, width, height);
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
    const float aspect = (float)width / (float)height;
    const float near = 0.01f;
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
    // View
    const float margin = 20.0f;
    const glm::mat4 view = glm::lookAt(light_position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::vec3 min_view = view * glm::vec4(min_bounds, 1.0f) + glm::vec4(margin);
    const glm::vec3 max_view = view * glm::vec4(max_bounds, 1.0f) - glm::vec4(margin);

    // Projection
    const float near_z = 0.01f;
    const float far_z = 100.0f;
    const float left = min_view.x;
    const float right = max_view.x;
    const float bottom = min_view.y;
    const float top = max_view.y;
    dbg(near_z, far_z, left, right, bottom, top);
    const glm::mat4 projection = glm::ortho(left, right, bottom, top, near_z, far_z);

    return projection * view;
}

Framebuffer::~Framebuffer()
{
    glDeleteFramebuffers(1, &framebuffer);
    delete texture;
}