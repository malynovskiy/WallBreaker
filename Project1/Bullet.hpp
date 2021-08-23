#pragma once
#include <glm/glm.hpp>

struct Bullet
{
  glm::vec2 position;
  glm::vec2 velocity;
  glm::vec2 acceleration;
  float radius;
  float mass;
  int id;
};
