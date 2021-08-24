#pragma once
#include "Bullet.hpp"
#include "Segment.hpp"

#include <SFML/Graphics.hpp>

#include <vector>
#include <mutex>

inline constexpr float DefaultBulletLifeTime = 5.0f;
inline constexpr float DefaultBulletRadius = 3.0f;
inline const sf::Color DefaultBulletColor = sf::Color::Black;

class WallBreaker;

class BulletManager
{
public:
  BulletManager() = default;
  BulletManager(sf::RenderWindow *window, float viewportWidth, float viewportHeight)
    : m_window(window), m_viewportWidth{ viewportWidth }, m_viewportHeight{ viewportHeight }
  {
  }
  ~BulletManager() = default;

  void Fire(glm::vec2 pos, glm::vec2 dir, float speed, float time, float lifetime = DefaultBulletLifeTime);
  void Update(float time);

  void SetViewportWidth(float width) { m_viewportWidth = width; }
  void SetViewportHeight(float height) { m_viewportHeight = height; }

  size_t GetNumberOfBullets() { return m_bullets.size(); }
  size_t GetNumberOfWalls() { return m_walls.size(); }

  void GenerateNewWalls()
  {
    constexpr float wallsThickness = 2.0f;
    const sf::Color wallsColor = sf::Color::Magenta;
    CreateWalls(wallsThickness, wallsColor);
  }

  void GenerateNewWalls(unsigned int ratio)
  {
    constexpr float wallsThickness = 2.0f;
    const sf::Color wallsColor = sf::Color::Cyan;
    CreateWalls(ratio, wallsThickness, wallsColor);
  }

private:
  int CreateBullet(glm::vec2 pos, float radius, float time, float lifetime, sf::Color color = DefaultBulletColor);
  inline void MoveBullet(Bullet &bullet, float dt);
  void ProcessBulletsCollision(float dt);

  // Walls stuff should be separated into a separate context for sure ASAP
  int CreateWall(glm::vec2 start_pos, glm::vec2 end_pos, float thickness, sf::Color color)
  {
    size_t i = m_walls.size();
    m_walls.push_back({ start_pos, end_pos, thickness });
    m_wallShapes.push_back(SegmentShape(m_walls[i].p0, m_walls[i].p1, color, thickness));
    return i;
  }
  void CreateWalls(float thickness, sf::Color color);
  void CreateWalls(unsigned int gridRatio, float thickness, sf::Color color);

private:
  std::mutex m_bulletsMutex;
  std::vector<Bullet> m_bullets;
  std::vector<sf::CircleShape> m_bulletShapes;

  std::vector<Segment> m_walls;
  std::vector<SegmentShape> m_wallShapes;

  float m_viewportWidth;
  float m_viewportHeight;

  float m_lastTimeStamp = 0.0f;
  sf::RenderWindow *m_window;
};
