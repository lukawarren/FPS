#include "render/renderer.h"

void run()
{
    Renderer renderer("FPS", 800, 600);
    World world;
    double frame_time = 0.0f;

    while (renderer.should_render())
    {
        // Delta time
        const double new_time = glfwGetTime();
        const double delta = new_time - frame_time;
        frame_time = new_time;

        ImGui::Begin("Debug");
        ImGui::Text("FPS: %0.3f - (%0.3f ms)", 1.0f / delta, delta * 1000.0f);
        ImGui::End();

        if (Window::window->get_key(GLFW_KEY_ESCAPE)) break;

        renderer.render(world);
    }
}

int main()
{
    run();
    glfwTerminate();
}