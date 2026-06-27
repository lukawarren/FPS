#include "common.h"
#include "render/renderer.h"

constexpr static inline int width = 1600;
constexpr static inline int height = 900;

int main()
{
    // Init Jolt
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
    JPH::TempAllocatorMalloc temp_allocator;

    Renderer renderer("FPS", width, height);

    while (renderer.update())
    {
        renderer.render();
    }

    delete JPH::Factory::sInstance;
}