#pragma once
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>

#include "Segment.hpp"
#include "Bullet.hpp"

#include <string>
#include <map>

// settings
inline constexpr uint16_t DefaultWindowWidth = 1920;
inline constexpr uint16_t DefaultWindowHeight = 1080;

class WallBreaker
{
public:
  WallBreaker(
    uint16_t width = DefaultWindowWidth, uint16_t height = DefaultWindowHeight, std::string title = "WallBreaker");
  ~WallBreaker() = default;

  void Start();
  inline bool IsRunning() const;

  uint16_t GetWidth() const { return m_width; }
  uint16_t GetHeight() const { return m_height; }

private:
  bool Update(float deltaTime);
  void Render();

  void Initialize();
  void Run();
  void ProcessInput(float deltaTime);

  void InitializeKeyMappings();

  void HandleMouseInput(sf::Event &e);

  int CreateBall(glm::vec2 v, float r = 3.0f, sf::Color color = sf::Color::Black);
  int CreateSegment(glm::vec2 start_pos, glm::vec2 end_pos, float thickness = 3.0f, sf::Color color = sf::Color::White);
  void CreateSegmentsField(float thickness = 3.0f, sf::Color color = sf::Color::White);
  void Fire(glm::vec2 pos, glm::vec2 dir, float speed, float time, float life_time);

private:

  std::vector<Bullet> vecBalls;
  std::vector<sf::CircleShape> vecBallShapes;
  Bullet *pSelectedBall = nullptr;

  std::vector<Segment> vecSegments;
  std::vector<LineSegmentShape> vecSegmentShapes;
  Segment *pSelectedSegment = nullptr;

  sf::RenderWindow m_window;
  sf::View m_view;

  // Preferences
  std::string m_windowTitle;
  uint16_t m_width;
  uint16_t m_height;
};
