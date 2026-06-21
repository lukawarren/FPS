#include "window.h"

Window::Window(const std::string& title, const int width, const int height, SDL_GPUDevice* device)
{
    // Create window
    const SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    window = SDL_CreateWindow(title.c_str(), width, height, flags);

    if (!window)
        throw std::runtime_error(
            "Failed to create window: " + std::string(SDL_GetError())
        );

#ifdef NDEBUG
    constexpr bool debug = false;
#else
    constexpr bool debug = true;
#endif

    // Attach to window
    if (!SDL_ClaimWindowForGPUDevice(device, window))
        throw std::runtime_error(
            "Failed to claim window: " + std::string(SDL_GetError())
        );
    this->device = device;
}

Window::~Window()
{
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyWindow(window);
}

void Window::capture_mouse()
{
    SDL_SetWindowMouseGrab(window, true);
    SDL_SetWindowRelativeMouseMode(window, true);
}

void Window::uncapture_mouse()
{
    SDL_SetWindowMouseGrab(window, false);
    SDL_SetWindowRelativeMouseMode(window, false);
}

glm::vec2 Window::get_mouse_position()
{
    float x, y;
    SDL_GetMouseState(&x, &y);
    return { x, y };
}

glm::vec2 Window::get_mouse_movement()
{
    float x, y;
    SDL_GetRelativeMouseState(&x, &y);
    return { x, y };
}

bool Window::get_key(const SDL_Scancode scancode)
{
    return key_states[scancode];
}

void Window::update()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
        {
            closed = true;
        }

        else if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_ESCAPE)
            {
                closed = true;
            }
        }
    }

    key_states = SDL_GetKeyboardState(NULL);
}
