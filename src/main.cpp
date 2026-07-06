#include "common.h"
#include "render/renderer.h"
#include "audio.h"

constexpr static inline int width = 1600;
constexpr static inline int height = 900;

int main()
{
    // Init Jolt
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();
    JPH::TempAllocatorMalloc temp_allocator;

    Audio audio;
    Renderer renderer("FPS", width, height, audio);
    srand((u32)time(NULL));

    while (renderer.update())
    {
        renderer.render();
        audio.update();
    }

    delete JPH::Factory::sInstance;
}