#pragma once
#include "window.h"
#include "world.h"
#include "render/shader.h"
#include "render/texture.h"

class Renderer
{
public:
    Renderer(const std::string& window_title, const int width, const int height);
    ~Renderer();

    bool should_render();
    void render(const World& world);

    Window window;
private:
    void render_forward_pass(
        const World& world,
        const glm::mat4& view_matrix,
        const glm::mat4& projection_matrix
    );

    // Shaders
    DiffuseShader diffuse_shader;
};