#include "WallBreaker.hpp"

#include <iostream>
#include <chrono>
#include <map>
#include <random>

WallBreaker::WallBreaker(uint16_t width, uint16_t height, std::string title)
  : m_width{ width }, m_height{ height }, m_windowTitle(title), m_window(sf::VideoMode(width, height), title),
    m_bulletManager(&m_window, width, height)
{
}

void WallBreaker::Initialize()
{
  constexpr float segmentsThickness = 3.0f;
  // CreateSegmentsField(segmentsThickness, sf::Color::Magenta);
}

void WallBreaker::Start()
{
  Initialize();
  Run();
}

void WallBreaker::Run()
{
  std::atomic_bool isWindowOpen = true;

  std::random_device os_seed;
  const auto seed = os_seed();
  std::mt19937 randGenerator(seed);
  std::uniform_int_distribution distributeX(0, (int)m_width);
  std::uniform_int_distribution distributeY(m_height / 2, (int)m_height);

  auto proc = [&randGenerator, &distributeX, &distributeY, &bulletManager = m_bulletManager, &clock = m_clock]() {
    auto randx = distributeX(randGenerator);
    auto randy = distributeY(randGenerator);
    auto id = std::hash<std::thread::id>{}(std::this_thread::get_id());
    bulletManager.Fire(glm::vec2(randx, randy),
      glm::normalize(glm::vec2(id * randx, id * randy)),
      1000,
      clock.getElapsedTime().asSeconds(),
      rand() % 6 + 1);
  };

  const size_t threadsCount = std::thread::hardware_concurrency() - 1;
  m_workThreads.reserve(threadsCount);
  for (size_t i = 0; i < threadsCount; ++i)
    m_workThreads.emplace_back([&isWindowOpen, &proc]() {
      while (isWindowOpen)
      {
        std::chrono::duration<float> msToSleep{ 0.1 };
        std::this_thread::sleep_for(msToSleep);

        proc();
      }
    });

  float temp{};
  float beginTimeStamp = m_clock.restart().asSeconds();
  float prevTimeStamp = m_clock.getElapsedTime().asSeconds();

  while (IsRunning())
  {
    const float currentTime = m_clock.getElapsedTime().asSeconds();
    const float deltaTime = currentTime - prevTimeStamp;
    prevTimeStamp = currentTime;
    temp += deltaTime;
    if (temp >= 0.35f)
    {
      m_window.setTitle(
        m_windowTitle + " - " + std::string("FPS: " + std::to_string(static_cast<uint16_t>(1.0 / deltaTime))));
      temp = 0;
    }
    m_window.clear();
    ProcessInput(deltaTime);
    Update(currentTime);
    Render();
  }

  isWindowOpen = false;

  for (auto &thread : m_workThreads)
    thread.join();
}

void WallBreaker::Update(float time)
{
  m_bulletManager.Update(time);
}

// bool WallBreaker::Update(float deltaTime)
// {
//   std::vector<std::pair<Bullet *, Bullet *>> vecCollidingPairs;
//   std::vector<Bullet *> vFakeBullets;

//   // Static collisions, i.e. overlap
//   for (auto &ball : vecBalls)
//   {
//     for (size_t j = 0; j < vecSegments.size(); ++j)
//     {
//       Segment &edge = vecSegments[j];
//       glm::vec2 v0 = edge.p1 - edge.p0;
//       glm::vec2 v1 = ball.position - edge.p0;

//       float len = glm::dot(v0, v0);
//       float t = std::max(0.0f, std::min(len, glm::dot(v0, v1))) / len;

//       glm::vec2 c = edge.p0 + v0 * t;

//       glm::vec2 n = ball.position - c;
//       float fDistance = glm::length(n);

//       if (fDistance <= (ball.radius + edge.thickness))
//       {
//         // Collision with a segment itself
//         if (t != 0 && t != 1)
//         {
//           ball.velocity = glm::reflect(ball.velocity, glm::normalize(n));
//         }
//         else
//         {
//           // Colision with the start/end of a segment
//           Bullet *fakeBullet = new Bullet;
//           fakeBullet->position = c;
//           fakeBullet->radius = edge.thickness;
//           fakeBullet->mass = ball.mass * 0.8f;
//           fakeBullet->velocity = -ball.velocity;

//           // Add collision to vector of collisions for dynamic resolution
//           vecCollidingPairs.push_back({ &ball, fakeBullet });
//           vFakeBullets.push_back(fakeBullet);
//           // Calculate displacement required
//           float fOverlap = 1.0f * (fDistance - ball.radius - fakeBullet->radius);

//           // Displace Current Ball away from collision
//           ball.position -= fOverlap * (ball.position - fakeBullet->position) / fDistance;
//         }

//         vecSegments.erase(vecSegments.begin() + j--);
//       }
//     }
//   }

//   for (auto &ball : vFakeBullets)
//     delete ball;
//   vFakeBullets.clear();

//   return true;
// }

void WallBreaker::Render()
{
  // This loop is a bottleneck right now because of SFML renderer, definitely should be reworked later
  //    TODO: Even primitive shapes in SFML contain a lot of useless fields which occupies a lot of memory we are
  //    wasting a lot of time on cache misses due to big size of rendering elements. For our current task we should use
  //    custom
  //     SFML shapes with smaller size (or use own OpenGL renderer) to pack primitive shapes mory tightly)
  // for (size_t i = 0; i < m_bulletManager.m_bullets.size(); ++i)
  //{
  //  auto &bullet = m_bulletManager.m_bullets[i];
  //  glm::vec2 displacedPos{ bullet.position.x, bullet.position.y };
  //  displacedPos -= bullet.radius;
  //  m_bulletManager.m_shapes[i].setPosition({ displacedPos.x, displacedPos.y });
  //  m_window.draw(m_bulletManager.m_shapes[i]);
  //}

  // no need to set position each time, need to check for displacement somehow
  for (size_t i = 0; i < vecSegments.size(); ++i)
  {
    vecSegmentShapes[i].setPosition(vecSegments[i].p0, vecSegments[i].p1);
    m_window.draw(vecSegmentShapes[i]);
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
          // Fire(glm::vec2(randx, randy), glm::normalize(glm::vec2(i * randx, i * randy)), 1000, 0, 0);
          glm::vec2 dir = glm::vec2(i * randx, i * randy);
          if (i == 0)
            dir = { 1.0f, 1.0f };
          m_bulletManager.Fire(
            glm::vec2(randx, randy), glm::normalize(dir), 1000, m_clock.getElapsedTime().asSeconds(), rand() % 5 + 2);
        }
      }
    }
  }
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

inline bool WallBreaker::IsRunning() const
{
  return m_window.isOpen();
}
