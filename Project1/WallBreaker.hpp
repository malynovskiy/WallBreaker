#pragma once
#include <SFML/Graphics.hpp>
#include <glm/glm.hpp>

#include "Line.hpp"

#include <string>
#include <map>

// settings
inline constexpr uint16_t DefaultWindowWidth = 1920;
inline constexpr uint16_t DefaultWindowHeight = 1080;

struct sBall
{
  glm::vec2 position;
  glm::vec2 velocity;
  glm::vec2 acceleration;
  float radius;
  float mass;

  int id;
};

class WallBreaker
{
public:
  WallBreaker(
    uint16_t width = DefaultWindowWidth, uint16_t height = DefaultWindowHeight, std::string title = "WallBreaker");
  ~WallBreaker() = default;

  void Start();
  bool Update(float deltaTime);
  void Render();

  inline bool IsRunning() const;

  uint16_t GetWidth() const { return m_width; }
  uint16_t GetHeight() const { return m_height; }

private:
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
  static std::map<int, bool> keyMap;

private:
  std::vector<sBall> vecBalls;
  std::vector<sf::CircleShape> vecBallShapes;
  sBall *pSelectedBall = nullptr;

  std::vector<sLineSegment> vecSegments;
  std::vector<LineSegmentShape> vecSegmentShapes;
  sLineSegment *pSelectedSegment = nullptr;

  sf::RenderWindow m_window;
  sf::View m_view;

  std::string m_windowTitle;
  uint16_t m_width;
  uint16_t m_height;
};
