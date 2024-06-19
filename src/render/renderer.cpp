#include "render/renderer.h"
#include "transform.h"

Renderer::Renderer(const std::string& window_title, const int width, const int height) :
    window(window_title, width, height)
{
    // Setup GL state
    glCullFace(GL_BACK);
    // glEnable(GL_CULL_FACE);
    dbg("todo: cull");
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Init ImGui
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.glfw_window, true);
    ImGui_ImplOpenGL3_Init("#version 150");
}

bool Renderer::should_render()
{
    // Update window
    if (!window.update()) return false;
    glViewport(0, 0, window.framebuffer_width, window.framebuffer_height);

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

    // Render normal geometry
    render_forward_pass(world, view_matrix, projection_matrix);

    // Render ImGui
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Renderer::render_forward_pass(
    const World& world,
    const glm::mat4& view_matrix,
    const glm::mat4& projection_matrix
)
{
    // Begin forward pass
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    diffuse_shader.bind();

    for (const Mesh* mesh : world.meshes)
    {
        static Transform t = Transform();
        diffuse_shader.set_uniform("matrix", projection_matrix * view_matrix * t.matrix());
        mesh->bind();
        mesh->draw();
    }
}

Renderer::~Renderer() {}
