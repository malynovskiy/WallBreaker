#pragma once
#include <glm/glm.hpp>

namespace Math
{

inline float dot(const glm::vec2 &v1, const glm::vec2 &v2)
{
  return v1.x * v2.x + v1.y * v2.y;
}

inline float length(const glm::vec2 &v)
{
  return std::sqrt(dot(v, v));
}

inline float distance(const glm::vec2 &v1, const glm::vec2 &v2)
{
  return length(v2 - v1);
}

inline glm::vec2 normalize(const glm::vec2 &v)
{
  return v * (1.0f / length(v));
}

inline glm::vec2 reflect(const glm::vec2 &i, const glm::vec2 &n)
{
  return i - 2.0f * dot(n, i) * n;
}

}// namespace Math
