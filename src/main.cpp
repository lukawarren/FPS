#include "common.h"
#include "renderer.h"

constexpr static inline int width = 800;
constexpr static inline int height = 600;

int main()
{
    Renderer renderer("FPS", width, height);

    while (renderer.update())
    {
        renderer.render();
    }
}