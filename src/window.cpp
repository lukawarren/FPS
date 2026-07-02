#include "window.h"

Window::Window(const std::string& title, const int width, const int height, SDL_GPUDevice* device)
{
    // Create window - NOTE: no SDL_WINDOW_HIGH_PIXEL_DENSITY for consistent artistic low-res look
    const SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | (
        QUALITY_SETTINGS.inverse_render_scale != 1 ? SDL_WINDOW_HIGH_PIXEL_DENSITY : 0
    );
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
    if (!key_states) return false;
    return key_states[scancode];
}

bool Window::get_key_pressed(const SDL_Scancode scancode)
{
    return just_pressed_keys.count(scancode) > 0;
}

bool Window::get_mouse_button(const uint8_t button)
{
    return (SDL_GetMouseState(nullptr, nullptr) & SDL_BUTTON_MASK(button)) != 0;
}

bool Window::get_mouse_button_pressed(const uint8_t button)
{
    return just_pressed_mouse_buttons.count(button) > 0;
}

void Window::update()
{
    just_pressed_keys.clear();
    just_pressed_mouse_buttons.clear();

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        ImGui_ImplSDL3_ProcessEvent(&event);

        if (event.type == SDL_EVENT_QUIT)
        {
            closed = true;
        }
        else if (event.type == SDL_EVENT_KEY_DOWN)
        {
            if (event.key.key == SDLK_Q)
            {
                closed = true;
            }

            if (event.key.repeat == 0 && !ImGui::GetIO().WantCaptureKeyboard)
            {
                just_pressed_keys.insert(event.key.scancode);
            }
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && !ImGui::GetIO().WantCaptureMouse)
        {
            just_pressed_mouse_buttons.insert(event.button.button);
        }
    }

    key_states = SDL_GetKeyboardState(NULL);
}
