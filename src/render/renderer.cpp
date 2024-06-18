#include "render/renderer.h"
#include "transform.h"

Renderer::Renderer(const std::string& window_title, const int width, const int height) :
    window(window_title, width, height)
{
    // Setup GL state
    glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

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
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    (void)view_matrix;
    (void)projection_matrix;
    (void)world;
}

Renderer::~Renderer() {}
