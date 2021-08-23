#include "BulletManager.hpp"
#include "WallBreaker.hpp"
namespace
{
inline float DoCirclesOverlap(glm::vec2 v1, float r1, glm::vec2 v2, float r2)
{
  const glm::vec2 v = v1 - v2;
  return fabs(glm::dot(v, v)) <= (r1 + r2) * (r1 + r2);
}
inline float IsPointInCircle(glm::vec2 c, float r1, glm::vec2 p)
{
  const glm::vec2 v = c - p;
  return fabs(glm::dot(v, v)) < (r1 * r1);
}
}// namespace

int BulletManager::CreateBullet(glm::vec2 pos, float radius, float time, float lifetime, sf::Color color)
{
  Bullet b{};
  b.position = pos;
  b.radius = radius;
  b.mass = radius * 10.0f;
  b.spawntime = time;
  b.lifetime = lifetime;

  m_bullets.emplace_back(b);

  sf::CircleShape shape{};
  shape.setRadius(radius);
  shape.setFillColor(color);
  shape.setOutlineColor(sf::Color::White);
  shape.setOutlineThickness(3);
  m_shapes.emplace_back(shape);

  return m_bullets.size() - 1;
}

void BulletManager::Fire(glm::vec2 pos, glm::vec2 dir, float speed, float time, float lifetime)
{
  std::lock_guard lock(m_bulletsMutex);

  const int i = CreateBullet(pos, DefaultBulletRadius, time, lifetime);
  m_bullets[i].velocity = dir * speed;
}

void BulletManager::Update(float time)
{
  const float deltaTime = time - m_lastTimeStamp;
  m_lastTimeStamp = time;

  std::lock_guard lock(m_bulletsMutex);

  for (size_t i = 0; i < m_bullets.size(); ++i)
  {
    auto & bullet = m_bullets[i];
     if (bullet.spawntime > time)
     {
       continue;
     }

    if (bullet.spawntime + bullet.lifetime < time)
    {
      m_bullets.erase(m_bullets.begin() + i);
      // probably a lot of cache misses would be here, need to be investigated
      m_shapes.erase(m_shapes.begin() + i);
      i--;
      continue;
    }

    // Global bullets movement
    MoveBullet(bullet, deltaTime);
  }

  ProcessBulletsCollision(deltaTime);

  // Each SFML circle shape position located outside of circle, we should displace it
  for (size_t i = 0; i < m_bullets.size(); ++i)
  {
    glm::vec2 displacedPos{ m_bullets[i].position.x, m_bullets[i].position.y };
    displacedPos -= m_bullets[i].radius;
    m_shapes[i].setPosition({ displacedPos.x, displacedPos.y });
    m_window->draw(m_shapes[i]);
  }
}

inline void BulletManager::MoveBullet(Bullet &bullet, float deltaTime)
{
  // Acceleration sumulation
  constexpr float ExternalForceCoeff = 0.8f;
   bullet.acceleration = -bullet.velocity * ExternalForceCoeff;

  // Update bullet physics
   bullet.velocity += bullet.acceleration * deltaTime;
  bullet.position += bullet.velocity * deltaTime;

  // Wrap bullets around the screen
  if (bullet.position.x < 0)
    bullet.position.x += m_viewportWidth;
  if (bullet.position.x >= m_viewportWidth)
    bullet.position.x -= m_viewportWidth;
  if (bullet.position.y < 0)
    bullet.position.y += m_viewportHeight;
  if (bullet.position.y >= m_viewportHeight)
    bullet.position.y -= m_viewportHeight;

  // Clamp velocity near zero
  if (fabs(glm::dot(bullet.velocity, bullet.velocity)) < 0.01f)
    bullet.velocity = { 0.0f, 0.0f };
}

void BulletManager::ProcessBulletsCollision(float deltaTime)
{
  std::vector<std::pair<Bullet *, Bullet *>> collidingBullets;
  // Bullets collision handling
  for (auto &bullet : m_bullets)
  {
    for (auto &targetBullet : m_bullets)
    {
      if (&bullet == &targetBullet)
        continue;

      if (DoCirclesOverlap(bullet.position, bullet.radius, targetBullet.position, targetBullet.radius))
      {
        // Collision has occured
        collidingBullets.push_back({ &bullet, &targetBullet });
        // Distance between bullet centers
        float fDistance = glm::distance(bullet.position, targetBullet.position);
        // Calculate displacement required
        float fOverlap = 0.5f * (fDistance - bullet.radius - targetBullet.radius);
        // Displace Current bullet away from collision
        bullet.position -= fOverlap * (bullet.position - targetBullet.position) / fDistance;
        // Displace Target bullet away from collision
        targetBullet.position += fOverlap * (bullet.position - targetBullet.position) / fDistance;
      }
    }
  }

  // Handle Bullet vs Bullet collisions
  for (auto &c : collidingBullets)
  {
    Bullet *b1 = c.first;
    Bullet *b2 = c.second;

    const float fDistance = glm::distance(b1->position, b2->position);
    glm::vec2 n = (b2->position - b1->position) / fDistance;

    glm::vec2 tangent = { -n.y, n.x };
    // Dot Product Tangent
    float dpTan1 = glm::dot(b1->velocity, tangent);
    float dpTan2 = glm::dot(b2->velocity, tangent);

    // Dot Product Normal
    float dpNorm1 = glm::dot(b1->velocity, n);
    float dpNorm2 = glm::dot(b2->velocity, n);

    // Conservation of momentum in 1D
    float m1 = (dpNorm1 * (b1->mass - b2->mass) + 2.0f * b2->mass * dpNorm2) / (b1->mass + b2->mass);
    float m2 = (dpNorm2 * (b2->mass - b1->mass) + 2.0f * b1->mass * dpNorm1) / (b1->mass + b2->mass);

    // Update ball velocities
    b1->velocity = tangent * dpTan1 + n * m1;
    b2->velocity = tangent * dpTan2 + n * m2;
  }
}
