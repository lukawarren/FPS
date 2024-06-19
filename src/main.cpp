#include "render/renderer.h"
#include "map.h"

void run()
{
    Renderer renderer("FPS", 800, 600);
    Window& window = renderer.window;
    window.capture_mouse();
    World world;
    double frame_time = 0.0f;
    bool captured = true;
    glm::vec2 mouse_position = window.mouse_position();

    Map* map = new Map("test");
    Mesh mesh(map->vertices, map->texture_coordinates, map->normals, map->indices);
    world.meshes.push_back(&mesh);
    delete map;

    while (renderer.should_render())
    {
        // Delta time
        const double new_time = glfwGetTime();
        const double delta = new_time - frame_time;
        frame_time = new_time;

        ImGui::Begin("Debug");
        ImGui::Text("FPS: %0.3f - (%0.3f ms)", 1.0f / delta, delta * 1000.0f);
        ImGui::End();
        renderer.render(world);

        // Deal with mouse grabbing
        if (window.get_key(GLFW_KEY_ESCAPE, false))
        {
            if (captured)
            {
                window.uncapture_mouse();
                captured = false;
            }
            else
            {
                window.capture_mouse();
                mouse_position = window.mouse_position();
                captured = true;
            }
        }

        if (!captured) continue;

        // WASD
        const float speed = 10.0f * delta;
        glm::vec3 movement = {};
        if (window.get_key(GLFW_KEY_W)) movement.z += 1.0f;
        if (window.get_key(GLFW_KEY_S)) movement.z -= 1.0f;
        if (window.get_key(GLFW_KEY_A)) movement.x -= 1.0f;
        if (window.get_key(GLFW_KEY_D)) movement.x += 1.0f;

        // Apply relative to rotation
        world.camera.position += glm::vec3 { sin(glm::radians(world.camera.yaw)), 0, -cos(glm::radians(world.camera.yaw)) } * movement.z * speed;
        world.camera.position += glm::vec3 { cos(glm::radians(world.camera.yaw)), 0,  sin(glm::radians(world.camera.yaw)) } * movement.x * speed;

        // Vertical movement
        if (window.get_key(GLFW_KEY_SPACE)) world.camera.position.y += 1.0f * speed;
        if (window.get_key(GLFW_KEY_LEFT_SHIFT)) world.camera.position.y -= 1.0f * speed;

        // Mouse
        glm::vec2 mouse_movement = window.mouse_position() - mouse_position;
        mouse_position = window.mouse_position();
        const float sensitivity = 0.1f;
        world.camera.yaw += mouse_movement.x * sensitivity;
        world.camera.pitch += mouse_movement.y * sensitivity;

        // Confine rotation
        world.camera.pitch = std::max(std::min(world.camera.pitch, 90.0f), -90.0f);

        if (Window::window->get_key(GLFW_KEY_ESCAPE)) break;
    }
}

int main()
{
    run();
    glfwTerminate();
}