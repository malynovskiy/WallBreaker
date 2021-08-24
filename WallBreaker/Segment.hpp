#pragma once
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>

struct Segment
{
  glm::vec2 p0;
  glm::vec2 p1;
  float thickness;
};

constexpr unsigned int SegmentVerticesNumber = 4;
constexpr float DefaultSegmentShapeThickness = 3.0f;

class SegmentShape : public sf::Drawable
{
public:
  SegmentShape(const sf::Vector2f &p0,
    const sf::Vector2f &p1,
    sf::Color c = sf::Color::White,
    float t = DefaultSegmentShapeThickness)
    : color{c}, thickness{t}
  {
    setPosition(p0, p1);
  }

  SegmentShape(
    const glm::vec2 &p0, const glm::vec2 &p1, sf::Color c = sf::Color::White, float t = DefaultSegmentShapeThickness)
    : SegmentShape(sf::Vector2f(p0.x, p0.y), sf::Vector2f(p1.x, p1.y), c, t)
  {
  }

  void draw(sf::RenderTarget &target, sf::RenderStates states) const
  {
    target.draw(vertices, SegmentVerticesNumber, sf::Quads);
  }

  void setPosition(const glm::vec2 &p0, const glm::vec2 &p1)
  {
    setPosition(sf::Vector2f(p0.x, p0.y), sf::Vector2f(p1.x, p1.y));
  }
  
  void setPosition(const sf::Vector2f &p0, const sf::Vector2f &p1)
  {
    sf::Vector2f direction = p1 - p0;
    sf::Vector2f unitDirection = direction / std::sqrt(direction.x * direction.x + direction.y * direction.y);
    sf::Vector2f unitPerpendicular(-unitDirection.y, unitDirection.x);

    sf::Vector2f offset = (thickness / 2.0f) * unitPerpendicular;

    vertices[0].position = p0 + offset;
    vertices[1].position = p1 + offset;
    vertices[2].position = p1 - offset;
    vertices[3].position = p0 - offset;

    for (int i = 0; i < SegmentVerticesNumber; ++i)
      vertices[i].color = color;
  }

private:
  sf::Vertex vertices[SegmentVerticesNumber];
  float thickness;
  sf::Color color;
};
