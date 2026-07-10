#include "flashlight.h"

Flashlight::Flashlight() : Spotlight({}, {}, glm::vec3(1.0f), 30.0f, 0.01f, 10.0f, 45.0f)
{}

void Flashlight::update(
    const glm::vec3 position,
    const float pitch,
    const float yaw
)
{
    this->position = position;

    const float pitch_rad = glm::radians(-pitch);
    const float yaw_rad = glm::radians(yaw - 90.0f);
    direction.x = std::cos(pitch_rad) * std::cos(yaw_rad);
    direction.y = std::sin(pitch_rad);
    direction.z = std::cos(pitch_rad) * std::sin(yaw_rad);

    this->position += direction * 0.3f;
}