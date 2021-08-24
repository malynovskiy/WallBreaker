#include "BulletManager.hpp"
#include "WallBreaker.hpp"

#include <random>

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
  //shape.setOutlineColor(sf::Color::Yellow);
  //shape.setOutlineThickness(1);
  m_bulletShapes.emplace_back(shape);

  return m_bullets.size() - 1;
}

void BulletManager::Fire(glm::vec2 pos, glm::vec2 dir, float speed, float time, float lifetime)
{
  std::lock_guard lock(m_bulletsMutex);

  const int i = CreateBullet(pos, DefaultBulletRadius, time, lifetime, sf::Color::Yellow);
  m_bullets[i].velocity = dir * speed;
}

void BulletManager::Update(float time)
{
  const float deltaTime = time - m_lastTimeStamp;
  m_lastTimeStamp = time;

  std::lock_guard lock(m_bulletsMutex);

  for (size_t i = 0; i < m_bullets.size(); ++i)
  {
    auto &bullet = m_bullets[i];
    // Other threads can spawn bullets earlier, so we need to wait
    if (bullet.spawntime > time)
    {
      continue;
    }

    if (bullet.spawntime + bullet.lifetime < time)
    {
      m_bullets.erase(m_bullets.begin() + i);
      // probably a lot of cache misses would be here, need to be investigated
      m_bulletShapes.erase(m_bulletShapes.begin() + i);
      i--;
      continue;
    }

    // Global bullet movement
    MoveBullet(bullet, deltaTime);
  }

  ProcessBulletsCollision(deltaTime);

  // This loop is a bottleneck right now because of SFML renderer, definitely should be reworked later
  //    TODO: Even primitive shapes in SFML contain a lot of useless fields which occupies a lot of memory. 
  //    We are wasting a lot of time on cache misses due to big size of rendering elements. For our current task we should use
  //    custom SFML shapes with smaller size (or use own OpenGL renderer) to pack primitive shapes mory tightly)
  // Each SFML circle shape position located outside of circle, we should displace it
  for (size_t i = 0; i < m_bullets.size(); ++i)
  {
    glm::vec2 displacedPos{ m_bullets[i].position.x, m_bullets[i].position.y };
    displacedPos -= m_bullets[i].radius;
    m_bulletShapes[i].setPosition({ displacedPos.x, displacedPos.y });
    m_window->draw(m_bulletShapes[i]);
  }

  // TODO: Walls stuff should be revisited
  for (size_t i = 0; i < m_walls.size(); ++i)
  {
    // TODO: no need to set position here each time, walls are static, we should set shape postions somewhere else
    m_wallShapes[i].setPosition(m_walls[i].p0, m_walls[i].p1);
    m_window->draw(m_wallShapes[i]);
  }
}

inline void BulletManager::MoveBullet(Bullet &bullet, float deltaTime)
{
  // Acceleration sumulation
  constexpr float ExternalForceCoeff = 0.8f;
  //bullet.acceleration = -bullet.velocity * ExternalForceCoeff;

  // Update bullet physics
  //bullet.velocity += bullet.acceleration * deltaTime;
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
  std::vector<Bullet *> fakeBullets;

  // Bullets collision handling
  for (auto &bullet : m_bullets)
  {
    if(m_processBulletsCollision)
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

    for (size_t j = 0; j < m_walls.size(); ++j)
    {
      Segment &edge = m_walls[j];
      const glm::vec2 v0 = edge.p1 - edge.p0;
      const glm::vec2 v1 = bullet.position - edge.p0;

      const float len = glm::dot(v0, v0);
      // Clamping distance between 0 and 1 to handle only segment collion, not the infinite line
      const float t = std::max(0.0f, std::min(len, glm::dot(v0, v1))) / len;

      glm::vec2 c = edge.p0 + v0 * t;

      glm::vec2 n = bullet.position - c;
      float fDistance = glm::length(n);

      if (fDistance <= (bullet.radius + edge.thickness))
      {
        // 0 or 1 are collisions with start/end points
        if (t == 0 || t == 1)
        {
          // Colision with the start/end of a segment
          Bullet *fakeBullet = new Bullet;
          fakeBullet->position = c;
          fakeBullet->radius = edge.thickness;
          fakeBullet->mass = bullet.mass * 0.8f;
          fakeBullet->velocity = -bullet.velocity;

          // Add collision to vector of collisions for dynamic resolution
          collidingBullets.push_back({ &bullet, fakeBullet });
          fakeBullets.push_back(fakeBullet);
          // Calculate displacement
          float fOverlap = 1.0f * (fDistance - bullet.radius - fakeBullet->radius);
          bullet.position -= fOverlap * (bullet.position - fakeBullet->position) / fDistance;
        }
        else
        {
          // Collision with the "flat" part of a segment
          bullet.velocity = glm::reflect(bullet.velocity, glm::normalize(n));
        }

        m_walls.erase(m_walls.begin() + j);
        // Erasing shapes here could be a performace bottleneck due to possible cache misses
        m_wallShapes.erase(m_wallShapes.begin() + j);
        j--;
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

    // Update bullet velocities
    b1->velocity = tangent * dpTan1 + n * m1;
    b2->velocity = tangent * dpTan2 + n * m2;
  }

  for (auto &b : fakeBullets)
    delete b;
}

void BulletManager::CreateWalls(unsigned int gridRatio, float thickness, sf::Color color)
{
  if (!m_walls.empty())
    return;

  const float rectWidth = m_viewportWidth / gridRatio;
  const float rectHeight = m_viewportHeight / gridRatio;

  std::random_device os_seed;
  const auto seed = os_seed();
  std::mt19937 randGenerator(seed);

  for (size_t i = 0; i < gridRatio; ++i)
  {
    for (size_t j = 0; j < gridRatio; ++j)
    {
      glm::vec2 currRectOrigin(j * rectWidth, i * rectHeight);

      const int minX = currRectOrigin.x;
      const int maxX = currRectOrigin.x + rectWidth;
      const int minY = currRectOrigin.y;
      const int maxY = currRectOrigin.y + rectHeight;

      std::uniform_int_distribution distributeX(minX, maxX);
      std::uniform_int_distribution distributeY(minY, maxY);

      const glm::vec2 startPoint(distributeX(randGenerator), distributeY(randGenerator));
      const glm::vec2 endPoint(distributeX(randGenerator), distributeY(randGenerator));

      if (startPoint == endPoint)
        continue;

      CreateWall(startPoint, endPoint, thickness, color);
    }
  }
}

void BulletManager::GenerateNewWalls(unsigned int ratio)
{
  {
    constexpr float wallsThickness = 2.0f;
    const sf::Color wallsColor = sf::Color::Cyan;
    CreateWalls(ratio, wallsThickness, wallsColor);
  }
}

void BulletManager::RemoveAllWalls()
{
  m_walls.clear();
  m_wallShapes.clear();
}
