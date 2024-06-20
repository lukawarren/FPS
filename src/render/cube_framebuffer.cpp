#include "render/cube_framebuffer.h"
#include "config.h"

CubeFramebuffer::CubeFramebuffer() : cubemap(Texture::Type::Cubemap, shadow_size, shadow_size)
{
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, cubemap.handle(), 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void CubeFramebuffer::bind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glViewport(0, 0, shadow_size, shadow_size);
}

void CubeFramebuffer::unbind() const
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

std::array<glm::mat4, 6> CubeFramebuffer::get_matrices(
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

CubeFramebuffer::~CubeFramebuffer()
{
    glDeleteFramebuffers(1, &framebuffer);
}