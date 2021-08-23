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

class WallBreaker
{
public:
  WallBreaker(
    uint16_t width = DefaultWindowWidth, uint16_t height = DefaultWindowHeight, std::string title = DefaultTitle);
  ~WallBreaker() = default;

  void Start();
  inline bool IsRunning() const;

  uint16_t GetWidth() const { return m_width; }
  uint16_t GetHeight() const { return m_height; }

private:
  void Update(float time);
  void Render();

  void Initialize();
  void Run();
  void ProcessInput(float deltaTime);

  void InitializeKeyMappings();

  int CreateSegment(glm::vec2 start_pos, glm::vec2 end_pos, float thickness = 3.0f, sf::Color color = sf::Color::White);
  void CreateSegmentsField(float thickness = 3.0f, sf::Color color = sf::Color::White);

private:
  std::vector<std::thread> m_workThreads;
  BulletManager m_bulletManager;

  std::vector<Segment> vecSegments;
  std::vector<LineSegmentShape> vecSegmentShapes;
  Segment *pSelectedSegment = nullptr;

  sf::RenderWindow m_window;
  sf::View m_view;
  sf::Clock m_clock;

  // Preferences
  std::string m_windowTitle;
  uint16_t m_width;
  uint16_t m_height;
};
