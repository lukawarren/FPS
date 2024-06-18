#include "camera.h"
#include "render/mesh.h"

struct World
{
    Camera camera;
    std::vector<Mesh*> meshes;
};
