#include "common.h"
#include "render/renderer.h"
#include "audio.h"

constexpr static inline int width = 800;
constexpr static inline int height = 600;

int main()
{
    // Init Jolt
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
    JPH::TempAllocatorMalloc temp_allocator;

    Audio audio;
    Renderer renderer("FPS", width, height, audio);

    while (renderer.update())
    {
        renderer.render();
        audio.update();
    }

    delete JPH::Factory::sInstance;
}