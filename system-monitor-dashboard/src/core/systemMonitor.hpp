#include "systemSnapshot.hpp"
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

class SystemMonitor {
public:
    explicit SystemMonitor(std::chrono::milliseconds interval);
    ~SystemMonitor();                         // must not block > 2× interval
    void start();
    void stop();
    SystemSnapshot latest() const;            // thread-safe read
private:
    std::jthread        m_thread;             // C++20
    std::atomic<bool>   m_running{false};
    mutable std::mutex  m_mutex;
    SystemSnapshot      m_latest;
};