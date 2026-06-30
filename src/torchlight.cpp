#include "torchlight.h"
#include "transform.h"

constexpr static inline float BRIGHTNESS_INTENSITY = 0.2f;

Torchlight::Torchlight(
    const glm::vec3 position,
    const glm::vec3 direction,
    const glm::vec3 colour,
    const float near,
    const float far,
    const float angle
) :
    Spotlight(position, direction, colour, near, far, angle),
    original_colour(colour),
    original_direction(direction)
{
    seed = (float)rand() / (float)RAND_MAX;
}

void Torchlight::update(const float time)
{
    colour = original_colour * get_flicker(time);
    direction = get_flicker_direction(time * 3.0f);
}

glm::mat4 Torchlight::get_model_matrix() const
{
    Transform t;
    t.position = position;
    t.rotation.y = glm::degrees(std::atan2(original_direction.x, original_direction.z)) - 90.0f;
    return t.matrix();
}

float Torchlight::get_flicker(const float time)
{
    float n = std::sin(time * 8.0f  + seed * 17.0f) * 0.35f
            + std::sin(time * 13.7f + seed * 3.1f)  * 0.25f
            + std::sin(time * 27.3f + seed * 91.0f) * 0.15f;

    float dip = std::sin(time * 0.6f + seed * 5.0f);
    if (dip > 0.92f) n -= (dip - 0.92f) * 4.0f;

    float intensity = 1.0f + n * BRIGHTNESS_INTENSITY;
    return glm::clamp(intensity, 0.55f, 1.15f);
}

glm::vec3 Torchlight::get_flicker_direction(const float time)
{
    glm::vec3 jitter(
        std::sin(time * 9.0f  + seed * 11.0f),
        std::sin(time * 7.3f  + seed * 23.0f) * 0.5f,
        std::sin(time * 12.1f + seed * 31.0f)
    );
    return glm::normalize(original_direction + jitter * glm::radians(0.4f));
}