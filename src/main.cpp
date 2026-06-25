#include "common.h"
#include "render/renderer.h"

constexpr static inline int width = 1600;
constexpr static inline int height = 900;

int main()
{
    Renderer renderer("FPS", width, height);

    while (renderer.update())
    {
        renderer.render();
    }
}