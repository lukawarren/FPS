#pragma once
#include "common.h"

class Window
{
public:
    Window(const std::string& title, const int width, const int height, SDL_GPUDevice* device);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    void update();

    bool should_close() const { return closed; }
    SDL_Window* get_window() const { return window; }

    void capture_mouse();
    void uncapture_mouse();
    glm::vec2 get_mouse_position();
    glm::vec2 get_mouse_movement();
    bool get_key(const SDL_Scancode scancode);

private:
    SDL_GPUDevice* device;
    SDL_Window* window = nullptr;
    bool closed = false;
    const bool* key_states;
};