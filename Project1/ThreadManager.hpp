#pragma once
#include <vector>
#include <thread>
#include <atomic>

class ThreadManager
{
public:
  ThreadManager() = default;
  ~ThreadManager() = default;

private:
  std::vector<std::thread> m_threads;
  std::atomic_bool m_threadsRunning;
};
