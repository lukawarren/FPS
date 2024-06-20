#include "render/renderer.h"
#include "transform.h"

Renderer::Renderer(const std::string& window_title, const int width, const int height) :
    window(window_title, width, height)
{
    // Setup GL state
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Init ImGui
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.glfw_window, true);
    ImGui_ImplOpenGL3_Init("#version 150");

    // Texture units
    diffuse_shader.bind();
    diffuse_shader.set_uniform("diffuse", 0);
    diffuse_shader.set_uniform("point_depth", 1);
}

void Renderer::load_world(const World& world)
{
    point_light_framebuffers.resize(world.point_lights.size());
    for (size_t i = 0; i < world.point_lights.size(); ++i)
        point_light_framebuffers[i] = new CubeFramebuffer();
}

bool Renderer::should_render()
{
    // Update window
    if (!window.update()) return false;

    // Begin ImGui
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    return true;
}

void Renderer::render(const World& world)
{
    // Get camera matrices
    const glm::mat4 projection_matrix = world.camera.projection_matrix(
        window.framebuffer_width,
        window.framebuffer_height
    );
    const glm::mat4 view_matrix = world.camera.view_matrix();

    // Shadow passes
    render_point_light_pass(world);

    // Render normal geometry
    render_forward_pass(world, view_matrix, projection_matrix);

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::render_point_light_pass(const World& world)
{
    assert(world.point_lights.size() == point_light_framebuffers.size());
    point_light_shader.bind();

    for (size_t i = 0; i < world.point_lights.size(); ++i)
    {
        CubeFramebuffer* framebuffer = point_light_framebuffers[i];
        framebuffer->bind();

        const auto matrices = framebuffer->get_matrices(
            world.point_lights[i].position,
            world.point_lights[i].distance
        );

        // Set uniforms
        point_light_shader.set_uniform("light_position", world.point_lights[i].position);
        point_light_shader.set_uniform("far_plane", world.point_lights[i].distance);
        point_light_shader.set_uniform("matrices[0]", matrices[0]);
        point_light_shader.set_uniform("matrices[1]", matrices[1]);
        point_light_shader.set_uniform("matrices[2]", matrices[2]);
        point_light_shader.set_uniform("matrices[3]", matrices[3]);
        point_light_shader.set_uniform("matrices[4]", matrices[4]);
        point_light_shader.set_uniform("matrices[5]", matrices[5]);
        point_light_shader.set_uniform("model", glm::mat4(1.0f));

        // Render
        glClear(GL_DEPTH_BUFFER_BIT);
        for (const auto& draw_call : world.map->draw_calls)
        {
            draw_call.mesh->bind();
            draw_call.mesh->draw();
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::render_forward_pass(
    const World& world,
    const glm::mat4& view_matrix,
    const glm::mat4& projection_matrix
)
{
    // Begin forward pass
    glViewport(0, 0, window.framebuffer_width, window.framebuffer_height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    diffuse_shader.bind();
    diffuse_shader.set_uniform("view_projection", projection_matrix * view_matrix);
    diffuse_shader.set_uniform("model", glm::mat4(1.0f));

    if (world.point_lights.size() > 0)
    {
        diffuse_shader.set_uniform("light_position", world.point_lights[0].position);
        diffuse_shader.set_uniform("light_colour", world.point_lights[0].colour);
        diffuse_shader.set_uniform("light_far_plane", world.point_lights[0].distance);
        point_light_framebuffers[0]->cubemap.bind(1);
    }

    for (const auto& draw_call : world.map->draw_calls)
    {
        draw_call.texture->bind();
        draw_call.mesh->bind();
        draw_call.mesh->draw();
    }
}

Renderer::~Renderer()
{
    for (CubeFramebuffer* fb : point_light_framebuffers)
        delete fb;
}
