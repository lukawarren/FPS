#pragma once
#include "window.h"
#include "world.h"
#include "render/shader.h"
#include "render/texture.h"
#include "render/cube_framebuffer.h"

class Renderer
{
public:
    Renderer(const std::string& window_title, const int width, const int height);
    ~Renderer();

    void load_world(const World& world);
    bool should_render();
    void render(const World& world);

    Window window;
private:
    void render_point_light_pass(const World& world, const bool render_only_dynamic);
    void render_forward_pass(
        const World& world,
        const glm::mat4& view_matrix,
        const glm::mat4& projection_matrix
    );

    // Shaders
    DiffuseShader diffuse_shader;
    PointLightShader point_light_shader;

    // Resources
    std::vector<CubeFramebuffer*> point_light_framebuffers;
    std::unordered_map<const char*, Texture*> textures;
    std::unordered_map<const char*, Mesh*> meshes;
};