#include "common.h"
#include "spotlight.h"

class Torchlight : public Spotlight
{
public:
    Torchlight(
        const glm::vec3 position,
        const glm::vec3 direction,
        const glm::vec3 colour,
        const float near,
        const float far,
        const float angle
    );

    void update(const float time);
    glm::mat4 get_model_matrix() const;

private:
    float get_flicker(const float time);
    glm::vec3 get_flicker_direction(const float time);

    glm::vec3 original_colour;
    glm::vec3 original_direction;
    float seed;
};
