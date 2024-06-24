#pragma once
#include "window.h"
#include "world.h"
#include "render/shader.h"
#include "render/texture.h"
#include "render/framebuffer.h"

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
    void render_directional_light_pass(const World& world, const bool render_only_dynamic);
    void render_forward_pass(
        const World& world,
        const glm::mat4& view_matrix,
        const glm::mat4& projection_matrix
    );
    void render_sky_pass(const glm::mat4& inverse_projection_view);

    // Shaders
    DiffuseShader diffuse_shader;
    PointLightShader point_light_shader;
    DirectionalLightShader directional_light_shader;
    SkyShader sky_shader;

    // Resources
    std::optional<Framebuffer> directional_light_framebuffer;
    std::vector<Framebuffer*> point_light_framebuffers;
    std::unordered_map<const char*, Texture*> textures;
    std::unordered_map<const char*, Mesh*> meshes;
    Texture dummy_cubemap;
    Mesh quad;

    // Cached info
    glm::vec3 min_world_bounds;
    glm::vec3 max_world_bounds;
};