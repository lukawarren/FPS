#include "common.h"
#include "spotlight.h"

class Flashlight : public Spotlight
{
public:
    Flashlight();

    void update(
        const glm::vec3 position,
        const float pitch,
        const float yaw
    );

    bool enabled = false;
};