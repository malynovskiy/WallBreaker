#include "WallBreaker.hpp"
#include <iostream>
#include <chrono>
#include <map>
#include <random>

using KeyPair = std::pair<int, bool>;
std::map<int, bool> WallBreaker::keyMap;

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

WallBreaker::WallBreaker(uint16_t width, uint16_t height, std::string title)
  : m_width{ width }, m_height{ height }, m_windowTitle(title), m_window(sf::VideoMode(width, height), title)
{
}

void WallBreaker::Initialize()
{
  constexpr float segmentsThickness = 3.0f;
  CreateSegmentsField(segmentsThickness, sf::Color::Magenta);
}

void WallBreaker::Start()
{
  Initialize();
  Run();
}

void WallBreaker::Fire(glm::vec2 pos, glm::vec2 dir, float speed, float time, float life_time)
{
  int id = CreateBall(pos, 5);
  vecBalls[id].velocity = dir * speed;
}

int WallBreaker::CreateBall(glm::vec2 v, float r, sf::Color color)
{
  Bullet b{};
  b.position = v;
  b.radius = r;
  b.mass = r * 10.0f;

  b.id = vecBalls.size();
  vecBalls.emplace_back(b);

  sf::CircleShape shape{};
  shape.setRadius(r);
  shape.setFillColor(color);
  shape.setOutlineColor(sf::Color::White);
  shape.setOutlineThickness(3);
  vecBallShapes.emplace_back(shape);

  return b.id;
}

int WallBreaker::CreateSegment(glm::vec2 start_pos, glm::vec2 end_pos, float thickness, sf::Color color)
{
  size_t i = vecSegments.size();
  vecSegments.push_back({ start_pos, end_pos, thickness });
  vecSegmentShapes.push_back(LineSegmentShape(vecSegments[i].p0, vecSegments[i].p1, color, thickness));
  return i;
}

void WallBreaker::CreateSegmentsField(float thickness, sf::Color color)
{
  constexpr uint16_t SegmentGridCols = 15;
  constexpr uint16_t SegmentGridRows = 10;

  const float rectWidth = m_width / SegmentGridCols;
  const float rectHeight = m_height / 2 / SegmentGridRows;

  std::random_device os_seed;
  const auto seed = os_seed();
  std::mt19937 randGenerator(seed);

  for (size_t i = 0; i < SegmentGridRows; ++i)
  {
    for (size_t j = 0; j < SegmentGridCols; ++j)
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

      CreateSegment(startPoint, endPoint, thickness, color);
    }
  }
}

void WallBreaker::Run()
{
  sf::Clock clock{};
  while (IsRunning())
  {
    float deltaTime = clock.restart().asSeconds();
    m_window.setTitle(
      m_windowTitle + " - " + std::string("FPS: " + std::to_string(static_cast<uint16_t>(1.0 / deltaTime))));

    ProcessInput(deltaTime);
    Update(deltaTime);
    Render();
  }
}

bool WallBreaker::Update(float deltaTime)
{
  std::vector<std::pair<Bullet *, Bullet *>> vecCollidingPairs;
  std::vector<Bullet *> vFakeBullets;
  // Update Ball Positions
  for (auto &ball : vecBalls)
  {
    // Add Drag to emulate rolling friction
    ball.acceleration = -ball.velocity * 0.8f;

    // Update ball physics
    ball.velocity += ball.acceleration * deltaTime;
    ball.position += ball.velocity * deltaTime;

    // Wrap the balls around screen
    if (ball.position.x < 0)
      ball.position.x += (float)m_width;
    if (ball.position.x >= m_width)
      ball.position.x -= (float)m_width;
    if (ball.position.y < 0)
      ball.position.y += (float)m_height;
    if (ball.position.y >= m_height)
      ball.position.y -= (float)m_height;

    // Clamp velocity near zero
    if (fabs(glm::dot(ball.velocity, ball.velocity)) < 0.01f)
    {
      ball.velocity = { 0.0f, 0.0f };
    }
  }

  // Static collisions, i.e. overlap
  for (auto &ball : vecBalls)
  {
    for (size_t j = 0; j < vecSegments.size(); ++j)
    {
      Segment &edge = vecSegments[j];
      glm::vec2 v0 = edge.p1 - edge.p0;
      glm::vec2 v1 = ball.position - edge.p0;

      float len = glm::dot(v0, v0);
      float t = std::max(0.0f, std::min(len, glm::dot(v0, v1))) / len;

      glm::vec2 c = edge.p0 + v0 * t;

      glm::vec2 n = ball.position - c;
      float fDistance = glm::length(n);

      if (fDistance <= (ball.radius + edge.thickness))
      {
        // Collision with a segment itself
        if (t != 0 && t != 1)
        {
          ball.velocity = glm::reflect(ball.velocity, glm::normalize(n));
        }
        else
        {
          // Colision with the start/end of a segment
          Bullet *fakeBullet = new Bullet;
          fakeBullet->position = c;
          fakeBullet->radius = edge.thickness;
          fakeBullet->mass = ball.mass * 0.8f;
          fakeBullet->velocity = -ball.velocity;

          // Add collision to vector of collisions for dynamic resolution
          vecCollidingPairs.push_back({ &ball, fakeBullet });
          vFakeBullets.push_back(fakeBullet);
          // Calculate displacement required
          float fOverlap = 1.0f * (fDistance - ball.radius - fakeBullet->radius);

          // Displace Current Ball away from collision
          ball.position -= fOverlap * (ball.position - fakeBullet->position) / fDistance;
        }

        vecSegments.erase(vecSegments.begin() + j--);
      }
    }

    for (auto &target : vecBalls)
    {
      if (ball.id != target.id)
      {
        if (DoCirclesOverlap(ball.position, ball.radius, target.position, target.radius))
        {
          // Collision has occured
          vecCollidingPairs.push_back({ &ball, &target });

          // Distance between ball centers
          float fDistance = glm::distance(ball.position, target.position);

          // Calculate displacement required
          float fOverlap = 0.5f * (fDistance - ball.radius - target.radius);

          // Displace Current Ball away from collision
          ball.position -= fOverlap * (ball.position - target.position) / fDistance;

          // Displace Target Ball away from collision
          target.position += fOverlap * (ball.position - target.position) / fDistance;
        }
      }
    }
  }

  // Now work out dynamic collisions
  for (auto c : vecCollidingPairs)
  {
    Bullet *b1 = c.first;
    Bullet *b2 = c.second;

    // Distance between balls
    float fDistance = glm::distance(b1->position, b2->position);

    // Normal
    glm::vec2 n = (b2->position - b1->position) / fDistance;

    // Tangent
    glm::vec2 t = { -n.y, n.x };

    // Dot Product Tangent
    float dpTan1 = glm::dot(b1->velocity, t);
    float dpTan2 = glm::dot(b2->velocity, t);

    // Dot Product Normal
    float dpNorm1 = glm::dot(b1->velocity, n);
    float dpNorm2 = glm::dot(b2->velocity, n);

    // Conservation of momentum in 1D
    float m1 = (dpNorm1 * (b1->mass - b2->mass) + 2.0f * b2->mass * dpNorm2) / (b1->mass + b2->mass);
    float m2 = (dpNorm2 * (b2->mass - b1->mass) + 2.0f * b1->mass * dpNorm1) / (b1->mass + b2->mass);

    // Update ball velocities
    b1->velocity = t * dpTan1 + n * m1;
    b2->velocity = t * dpTan2 + n * m2;
  }

  for (auto &ball : vFakeBullets)
    delete ball;
  vFakeBullets.clear();

  return true;
}

void WallBreaker::Render()
{
  m_window.clear();

  // This loop is a bottleneck right now because of SFML renderer, definitely should be reworked later
  //    TODO: Even primitive shapes in SFML contain a lot of useless fields which occupies a lot of memory we are
  //    wasting a lot of
  //            time on cache misses due to big size of rendering elements. For out current task we should use custom
  //            SFML shapes with smaller size (or use own OpenGL renderer) to pack primitive shapes mory tightly)
  for (size_t i = 0; i < vecBalls.size(); ++i)
  {
    glm::vec2 displacedPos{ vecBalls[i].position.x, vecBalls[i].position.y };
    displacedPos -= vecBalls[i].radius;
    vecBallShapes[i].setPosition({ displacedPos.x, displacedPos.y });
    m_window.draw(vecBallShapes[i]);
  }
  // no need to set position each time, need to check for displacement somehow
  for (size_t i = 0; i < vecSegments.size(); ++i)
  {
    vecSegmentShapes[i].setPosition(vecSegments[i].p0, vecSegments[i].p1);
    m_window.draw(vecSegmentShapes[i]);
  }

  if (pSelectedBall != nullptr && sf::Mouse::isButtonPressed(sf::Mouse::Right))
  {
    sf::Vector2f ballPos = { pSelectedBall->position.x, pSelectedBall->position.y };
    auto mousePos = sf::Mouse::getPosition(m_window);
    LineSegmentShape line(ballPos, sf::Vector2f(mousePos.x, mousePos.y), sf::Color::Cyan);
    m_window.draw(line);
  }

  m_window.display();
}

void WallBreaker::ProcessInput(float deltaTime)
{
  sf::Event event;
  while (m_window.pollEvent(event))
  {
    if (event.type == sf::Event::Closed)
      m_window.close();

    HandleMouseInput(event);

    static std::random_device os_seed;
    static const auto seed = os_seed();
    static std::mt19937 randGenerator(seed);

    static std::uniform_int_distribution distributeX(0, (int)m_width);
    static std::uniform_int_distribution distributeY(m_height / 2, (int)m_height);

    if (event.type == sf::Event::KeyPressed)
    {
      if (event.key.code == sf::Keyboard::A)
      {
        for (int i = 0; i < 100; ++i)
        {
          auto randx = distributeX(randGenerator);
          auto randy = distributeY(randGenerator);
          Fire(glm::vec2(randx, randy), glm::normalize(glm::vec2(i * randx, i * randy)), 1000, 0, 0);
        }
      }
    }
  }
}

void WallBreaker::HandleMouseInput(sf::Event &e)
{
  if (e.type == sf::Event::MouseButtonPressed)
  {
    pSelectedBall = nullptr;
    const auto mousePos = sf::Mouse::getPosition(m_window);
    std::cout << "x: " << mousePos.x << "\ty:" << mousePos.y << '\n';
    for (auto &bullet : vecBalls)
    {
      if (IsPointInCircle(bullet.position, bullet.radius, { mousePos.x, mousePos.y }))
      {
        pSelectedBall = &bullet;
        break;
      }
    }
  }

  if (e.type == sf::Event::MouseMoved)
  {
    if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
    {
      if (pSelectedBall != nullptr)
      {
        const auto mousePos = sf::Mouse::getPosition(m_window);
        pSelectedBall->position = { mousePos.x, mousePos.y };
      }
    }
  }

  if (e.type == sf::Event::MouseButtonReleased)
  {
    if (e.mouseButton.button == sf::Mouse::Left)
      pSelectedBall = nullptr;
    else
    {
      if (pSelectedBall != nullptr)
      {
        // Apply velocity
        const auto mousePos = sf::Mouse::getPosition(m_window);
        pSelectedBall->velocity = 5.0f * (pSelectedBall->position - glm::vec2(mousePos.x, mousePos.y));
      }

      pSelectedBall = nullptr;
    }
  }
}

inline bool WallBreaker::IsRunning() const
{
  return m_window.isOpen();
}
