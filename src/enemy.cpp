#include "enemy.h"
#include "player.h"

Enemy::Enemy(const glm::vec3 position) : sprite(
    position,
    { 1.0f, 0.0f, 0.0f },
    Sprite::ID::ENEMY
)
{
    sprite.transform.scale = {
        ENEMY_RADIUS * 2.0f,
        ENEMY_HEIGHT / 2.0f,
        ENEMY_RADIUS * 2.0f
    };

    sprite.transform.position.y += ENEMY_HEIGHT / 2.0f;
}

void Enemy::update(const glm::vec3 direction)
{
    sprite.face(direction * glm::vec3(1.0f, 0.0f, 1.0f));
}