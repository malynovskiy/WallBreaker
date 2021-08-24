#pragma once
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>

#include "Segment.hpp"
#include "BulletManager.hpp"

#include <string>
#include <map>
#include <thread>

// settings
inline constexpr uint16_t DefaultWindowWidth = 1920;
inline constexpr uint16_t DefaultWindowHeight = 1080;
inline constexpr auto DefaultTitle = "WallBreaker";

constexpr uint16_t DefaultBulletSpeed = 100;

class WallBreaker
{
public:
  WallBreaker(
    uint16_t width = DefaultWindowWidth, uint16_t height = DefaultWindowHeight, std::string title = DefaultTitle);
  ~WallBreaker() = default;

  void Run();
  inline bool IsRunning() const;

  uint16_t GetWidth() const { return m_width; }
  uint16_t GetHeight() const { return m_height; }

private:
  void Update(float time);
  void ProcessInput();

private:
  std::vector<std::thread> m_workThreads;
  std::atomic_bool m_workThreadsRunning;
  BulletManager m_bulletManager;

  // TODO: Renderer should be separated into another context with some interface
  sf::RenderWindow m_window;
  sf::View m_view;
  sf::Clock m_clock;

  // Preferences
  std::string m_windowTitle;
  uint16_t m_width;
  uint16_t m_height;
};
