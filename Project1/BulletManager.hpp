#pragma once
#include "Bullet.hpp"

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

private:
  int CreateBullet(glm::vec2 pos, float radius, float time, float lifetime, sf::Color color = DefaultBulletColor);
  inline void MoveBullet(Bullet &bullet, float dt);
  void ProcessBulletsCollision(float dt);

private:
  std::mutex m_bulletsMutex;
  std::vector<Bullet> m_bullets;
  std::vector<sf::CircleShape> m_shapes;

  float m_viewportWidth;
  float m_viewportHeight;

  float m_lastTimeStamp = 0.0f;
  sf::RenderWindow *m_window;
};
