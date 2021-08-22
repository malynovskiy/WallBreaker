#pragma once
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>

struct sLineSegment
{
  glm::vec2 p0;
  glm::vec2 p1;
  float thickness;
};

class LineSegmentShape : public sf::Drawable
{
public:
  LineSegmentShape(const sf::Vector2f &p0, const sf::Vector2f &p1, sf::Color c = sf::Color::White, float t = 5.0f)
    : color(c), thickness(t)
  {
    setPosition(p0, p1);
  }

  LineSegmentShape(const glm::vec2 &p0, const glm::vec2 &p1, sf::Color c = sf::Color::White, float t = 5.0f)
    : LineSegmentShape(sf::Vector2f(p0.x, p0.y), sf::Vector2f(p1.x, p1.y), c, t)
  {
  }

  void draw(sf::RenderTarget &target, sf::RenderStates states) const { target.draw(vertices, 4, sf::Quads); }

  void setPosition(const glm::vec2 &p0, const glm::vec2 &p1)
  {
    setPosition(sf::Vector2f(p0.x, p0.y), sf::Vector2f(p1.x, p1.y));
  }
  void setPosition(const sf::Vector2f &p0, const sf::Vector2f &p1)
  {
    sf::Vector2f direction = p1 - p0;
    sf::Vector2f unitDirection = direction / std::sqrt(direction.x * direction.x + direction.y * direction.y);
    sf::Vector2f unitPerpendicular(-unitDirection.y, unitDirection.x);

    sf::Vector2f offset = (thickness / 2.f) * unitPerpendicular;

    vertices[0].position = p0 + offset;
    vertices[1].position = p1 + offset;
    vertices[2].position = p1 - offset;
    vertices[3].position = p0 - offset;

    for (int i = 0; i < 4; ++i)
      vertices[i].color = color;
  }

private:
  sf::Vertex vertices[4];
  float thickness;
  sf::Color color;
};
