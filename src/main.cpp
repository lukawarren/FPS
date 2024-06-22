#include "render/renderer.h"
#include "map.h"

void run()
{
    // Setup
    Renderer renderer("FPS", 1600, 900);
    renderer.window.capture_mouse();

    // Load world
    World world;
    world.load_map("lights");
    renderer.load_world(world);
    world.player.setup_physics(world.map->world);

    double frame_time = 0.0f;
    bool captured = true;
    bool free_cam = false;

    while (renderer.should_render())
    {
        // Delta time
        const double new_time = glfwGetTime();
        const double delta = new_time - frame_time;
        frame_time = new_time;

        ImGui::Begin("Debug");
        ImGui::Text("FPS: %0.3f - (%0.3f ms)", 1.0f / delta, delta * 1000.0f);
        ImGui::End();

        if (renderer.window.get_key(GLFW_KEY_ESCAPE, false))
        {
            if (captured)
            {
                renderer.window.uncapture_mouse();
                captured = false;
            }
            else
            {
                renderer.window.capture_mouse();
                captured = true;
            }
        }

        if (renderer.window.get_key(GLFW_KEY_V, false))
            free_cam = !free_cam;

        world.player.update(world.map->world, world.camera, delta);

        if (captured)
        {
            if (free_cam)
                world.camera.update_freecam(delta);

            else
                world.player.update_camera(world.camera);
        }

        renderer.render(world);

        if (Window::window->get_key(GLFW_KEY_Q)) break;
    }
}

int main()
{
    run();
    glfwTerminate();
}