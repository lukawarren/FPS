#include "window.h"

Window* Window::window = nullptr;

Window::Window(const std::string& name, const int _width, const int _height) :
    width(_width), height(_height)
{
    // Create window and accompanying OpenGL context
    init_glfw(name);
    init_glad();

#ifndef __APPLE__
#ifndef NDEBUG
    // On modern platforms (i.e. not Apple), we can enable GL_DEBUG_OUTPUT
    // to catch errors that might otherwise go unnoticed.
    enable_debugging();
#endif
#endif

    window = this;
    cached_mouse_position = mouse_position();
}

bool Window::update()
{
#ifdef __APPLE__
    // Fix stuttering
    glFinish();
#endif

    // Mouse state
    cached_mouse_buttons[0] = get_mouse_button(GLFW_MOUSE_BUTTON_LEFT);
    cached_mouse_buttons[1] = get_mouse_button(GLFW_MOUSE_BUTTON_RIGHT);
    cached_mouse_buttons[2] = get_mouse_button(GLFW_MOUSE_BUTTON_MIDDLE);

    // Keyboard state
    for (int i = 0; i <= GLFW_KEY_LAST; ++i)
        cached_keyboard_buttons[i] = glfwGetKey(glfw_window, i) == GLFW_PRESS;

    cached_mouse_position = mouse_position();

    glfwSwapBuffers(glfw_window);
    glfwPollEvents();

    // Update window size for next frame
    glfwGetWindowSize(glfw_window, &width, &height);
    glfwGetFramebufferSize(glfw_window, &framebuffer_width, &framebuffer_height);

    return !glfwWindowShouldClose(glfw_window);
}

void Window::set_title(const std::string& title) const
{
    glfwSetWindowTitle(glfw_window, title.c_str());
}

bool Window::get_key(const int key, const bool repeat) const
{
    if (repeat == false && key >= 0 && key <= GLFW_KEY_LAST)
        return glfwGetKey(glfw_window, key) == GLFW_PRESS && !cached_keyboard_buttons[key];

    return glfwGetKey(glfw_window, key) == GLFW_PRESS;
}

bool Window::get_mouse_button(const int button, const bool repeat) const
{
    if (repeat == false && button >= 0 && button <= 2)
        return glfwGetMouseButton(glfw_window, button) == GLFW_PRESS && !cached_mouse_buttons[button];

    return glfwGetMouseButton(glfw_window, button) == GLFW_PRESS;
}

glm::vec2 Window::mouse_position() const
{
    double x, y;
    glfwGetCursorPos(glfw_window, &x, &y);
    return { x, y };
}

glm::vec2 Window::mouse_movement() const
{
    return mouse_position() - cached_mouse_position;
}

void Window::capture_mouse() const
{
    glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void Window::uncapture_mouse() const
{
    glfwSetInputMode(glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void Window::init_glfw(const std::string& name)
{
    // Init OpenGL core (>= 4.3 for debugging output and compute shaders)
    glfwInit();
#ifndef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
#endif
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
    glfwWindowHint(GLFW_RESIZABLE, true);

    // Create window
    glfw_window = glfwCreateWindow(width, height, name.c_str(), NULL, NULL);
    if (!glfw_window) throw std::runtime_error("unable to create GLFW window");

    // Get monitors...
    int n_monitors;
    GLFWmonitor** monitors = glfwGetMonitors(&n_monitors);
    if (n_monitors == 0) throw std::runtime_error("no monitors found");

    // ...get monitor resolution and position...
    const GLFWvidmode* mode = glfwGetVideoMode(monitors[0]);
    int monitor_x, monitor_y;
    glfwGetMonitorPos(monitors[0], &monitor_x, &monitor_y);

    // ...all to centre the window
    glfwSetWindowPos(
        glfw_window,
        monitor_x + (mode->width - width) / 2,
        monitor_y + (mode->height - height) / 2
    );

    // Get *framebuffer* size (may differ on retina displays, etc.)
    glfwGetFramebufferSize(glfw_window, &framebuffer_width, &framebuffer_height);

    // Bind window to OpenGL context and setup vsync
    glfwMakeContextCurrent(glfw_window);
    glfwSwapInterval(1);
}

void Window::init_glad()
{
    // GLAD will load OpenGL for us
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        throw std::runtime_error("failed to initialise GLAD");
}

void Window::enable_debugging()
{
    // Load debug context synchronously (i.e. raise errors as soon as they happen)
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

    glDebugMessageCallback([]
    (
        GLenum source,
        GLenum type,
        unsigned int id,
        GLenum severity,
        GLsizei,
        const char* message,
        const void*
        )
    {
        // Ignore certain "non-errors" (like Nvidia's "buffer successfully created")
        if (id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

        std::stringstream ss;
        ss << "id: " << id << " - ";

        switch (source)
        {
            case GL_DEBUG_SOURCE_API:             ss << "source: API";                   break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   ss << "source: window system";         break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER: ss << "source: shader compiler";       break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:     ss << "source: third Party";           break;
            case GL_DEBUG_SOURCE_APPLICATION:     ss << "source: application";           break;
            case GL_DEBUG_SOURCE_OTHER:           ss << "source: other";                 break;
            default:                              ss << "source: unknown";               break;
        }

        ss << " - ";

        switch (type)
        {
            case GL_DEBUG_TYPE_ERROR:               ss << "type: error";                 break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: ss << "type: deprecated Behaviour";  break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  ss << "type: undefined Behaviour";   break;
            case GL_DEBUG_TYPE_PORTABILITY:         ss << "type: portability";           break;
            case GL_DEBUG_TYPE_PERFORMANCE:         ss << "type: performance";           break;
            case GL_DEBUG_TYPE_MARKER:              ss << "type: marker";                break;
            case GL_DEBUG_TYPE_PUSH_GROUP:          ss << "type: push Group";            break;
            case GL_DEBUG_TYPE_POP_GROUP:           ss << "type: pop Group";             break;
            case GL_DEBUG_TYPE_OTHER:               ss << "type: other";                 break;
            default:                                ss << "type: unknown";               break;
        }

        ss << " - ";

        switch (severity)
        {
            case GL_DEBUG_SEVERITY_HIGH:         ss << "severity: high";                 break;
            case GL_DEBUG_SEVERITY_MEDIUM:       ss << "severity: medium";               break;
            case GL_DEBUG_SEVERITY_LOW:          ss << "severity: low";                  break;
            case GL_DEBUG_SEVERITY_NOTIFICATION: ss << "severity: notification";         break;
            default:                             ss << "severity: unknown";              break;
        }

        ss << " - message: " << message;
        dbg(ss.str());
    }, nullptr);

    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
}

Window::~Window()
{
    // Do not call glfwTerminate() here as then other resources (shaders, etc.)
    // owned by the renderer will not get a chance to be destroyed
}
