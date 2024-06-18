#include "render/renderer.h"
#include "map.h"

void run()
{
    Renderer renderer("FPS", 800, 600);
    World world;
    double frame_time = 0.0f;

    std::vector<float> vertices = Map("simple").get_vertices();
    Mesh mesh(vertices);
    world.meshes.push_back(&mesh);
    world.camera.position.z = 3;
    world.camera.position.y = 2;
    world.camera.pitch = 20.0f;

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