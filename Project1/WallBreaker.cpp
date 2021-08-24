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

void WallBreaker::Run()
{
  m_workThreadsRunning = true;

  std::random_device os_seed;
  const auto seed = os_seed();
  std::mt19937 randGenerator(seed);
  std::uniform_int_distribution distributeX(0, (int)m_width);
  std::uniform_int_distribution distributeY(m_height / 2, (int)m_height);

  // Initial test state with 1024 walls on a scene
  m_bulletManager.GenerateNewWalls(32);

  // Initial test proc for firing bullets each 20 milliseconds concurrently
  auto fireProcWithInterval =
    [&randGenerator, &distributeX, &distributeY, &bulletManager = m_bulletManager, &clock = m_clock]() {
      constexpr float bulletFireInterval = 0.02f;
      std::chrono::duration<float> msToSleep{ bulletFireInterval };
      std::this_thread::sleep_for(msToSleep);

      const auto randX = distributeX(randGenerator);
      const auto randY = distributeY(randGenerator);
      const glm::vec2 pos = glm::vec2(randX, randY);

      int id = std::hash<std::thread::id>{}(std::this_thread::get_id());
      int x = randX % 2 == 0 ? -id : id;
      int y = randY % 3 == 0 ? id : -id;
      const glm::vec2 dir = glm::vec2(pos.x * x, pos.y * y);

      bulletManager.Fire(
        pos, glm::normalize(dir), DefaultBulletSpeed, clock.getElapsedTime().asSeconds(), rand() % 6 + 1);
    };

  const size_t threadsCount = std::thread::hardware_concurrency() - 1;
  m_workThreads.reserve(threadsCount);
  for (size_t i = 0; i < threadsCount; ++i)
    m_workThreads.emplace_back([&threadRunning = m_workThreadsRunning, &fireProcWithInterval]() {
      while (threadRunning)
        fireProcWithInterval();
    });

  constexpr float showDebugInfoTimeRatio = 0.1f;
  float showDebugInfoTime{};
  float beginTimeStamp = m_clock.restart().asSeconds();
  float prevTimeStamp = m_clock.getElapsedTime().asSeconds();

  while (IsRunning())
  {
    const float currentTime = m_clock.getElapsedTime().asSeconds();
    const float deltaTime = currentTime - prevTimeStamp;
    prevTimeStamp = currentTime;
    showDebugInfoTime += deltaTime;

    m_window.clear();
    ProcessInput();
    m_bulletManager.Update(currentTime);
    m_window.display();

    if (showDebugInfoTime >= showDebugInfoTimeRatio)
    {
      const auto fpsStr = "FPS: " + std::to_string(static_cast<uint16_t>(1.0 / deltaTime));
      const auto bulletsNumStr = "Number of bullets: " + std::to_string(m_bulletManager.GetNumberOfBullets());
      const auto wallsNumStr = "Number of walls: " + std::to_string(m_bulletManager.GetNumberOfWalls());
      m_window.setTitle(m_windowTitle + " - " + fpsStr + " - " + bulletsNumStr + " - " + wallsNumStr);
      showDebugInfoTime = 0;
    }
  }

  m_workThreadsRunning = false;

  for (auto &thread : m_workThreads)
    thread.join();
}

void WallBreaker::ProcessInput()
{
  sf::Event event;
  while (m_window.pollEvent(event))
  {
    if (event.type == sf::Event::Closed)
      m_window.close();

    static std::random_device os_seed;
    static const auto seed = os_seed();
    static std::mt19937 randGenerator(seed);

    static std::uniform_int_distribution distributeX(0, static_cast<int>(m_width));
    static std::uniform_int_distribution distributeY(m_height / 2, static_cast<int>(m_height));

    auto bulletsGenerateProc = [&bulletManager = m_bulletManager, &clock = m_clock](size_t iterationsPerThread) {
      for (size_t i = 0; i < iterationsPerThread; ++i)
      {
        const auto randX = distributeX(randGenerator);
        const auto randY = distributeY(randGenerator);
        const glm::vec2 pos = glm::vec2(randX, randY);
        int id = std::hash<std::thread::id>{}(std::this_thread::get_id()) * i;
        int x = randX % 2 == 0 ? -id : id;
        int y = randY % 3 == 0 ? id : -id;
        const glm::vec2 dir = glm::vec2(pos.x * x, pos.y * y);

        bulletManager.Fire(
          pos, glm::normalize(dir), DefaultBulletSpeed, clock.getElapsedTime().asSeconds(), rand() % 7 + 3);
      }
    };

    if (event.type == sf::Event::KeyPressed)
    {
      if (event.key.code == sf::Keyboard::F1)
      {
        m_bulletManager.ToggleProcessBulletsCollision();
      }

      // Performance Stress Testing 1 - Generating 100 bullets
      if (event.key.code == sf::Keyboard::Z)
      {
        constexpr size_t bulletsCount = 100;

        m_workThreadsRunning = false;
        for (auto &thread : m_workThreads)
          thread.join();
        m_workThreads.clear();

        const size_t threadsCount = std::thread::hardware_concurrency() - 1;
        m_workThreads.reserve(threadsCount);
        const size_t iterationsPerThread = bulletsCount / threadsCount;
        for (size_t i = 0; i < threadsCount; ++i)
          m_workThreads.emplace_back(bulletsGenerateProc, iterationsPerThread);
      }

      // Performance Stress Testing 1 - Generating 100 walls
      if (event.key.code == sf::Keyboard::A)
      {
        m_bulletManager.GenerateNewWalls(10);
      }


      // Performance Stress Testing 2 - Generating 1000 bullets
      if (event.key.code == sf::Keyboard::X)
      {
        constexpr size_t bulletsCount = 1000;

        m_workThreadsRunning = false;
        for (auto &thread : m_workThreads)
          thread.join();
        m_workThreads.clear();

        const size_t threadsCount = std::thread::hardware_concurrency() - 1;
        m_workThreads.reserve(threadsCount);
        const size_t iterationsPerThread = bulletsCount / threadsCount;
        for (size_t i = 0; i < threadsCount; ++i)
          m_workThreads.emplace_back(bulletsGenerateProc, iterationsPerThread);
      }

      // Performance Stress Testing 2 - Generating 1000 walls
      if (event.key.code == sf::Keyboard::S)
      {
        // 32 x 32 = 1024 walls
        m_bulletManager.GenerateNewWalls(32);
      }

      if (event.key.code == sf::Keyboard::D)
      {
        m_bulletManager.RemoveAllWalls();
      }
    }
  }
}

inline bool WallBreaker::IsRunning() const
{
  return m_window.isOpen();
}
