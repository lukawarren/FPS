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

private:
    SDL_GPUDevice* device;
    SDL_Window* window = nullptr;
    bool closed = false;
};