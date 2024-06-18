#pragma once
#include "pch.h"

class Window
{
public:
    Window(const std::string& name, const int width, const int height);
    ~Window();

    bool update();
    void set_title(const std::string& title) const;

    // I/O
    bool get_key(const int key, const bool repeat = true) const;
    bool get_mouse_button(const int button, const bool repeat = true) const;
    glm::vec2 mouse_position() const;
    void capture_mouse() const;
    void uncapture_mouse() const;

    int width;
    int height;
    int framebuffer_width;
    int framebuffer_height;
    GLFWwindow* glfw_window;

    static Window* window;

private:
    void init_glfw(const std::string& name);
    void init_glad();
    void enable_debugging();

    bool cached_mouse_buttons[3] = {};
    bool cached_keyboard_buttons[GLFW_KEY_LAST + 1] = {};
};