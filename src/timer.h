// 
// timer.h
// ConwaysGameOfLife
// 
// Noah Hitz 2025
// 

#ifndef TIMER_H
#define TIMER_H

#include <chrono>

class Timer {
    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_end;
        bool m_isRunning = false;
        
    public:
        Timer() {}

        void start() {
            m_start = std::chrono::high_resolution_clock::now();
            m_isRunning = true;
        }

        void stop() { 
            m_end = std::chrono::high_resolution_clock::now();
            m_isRunning = false;
        }

        void resume() {
            m_isRunning = true;
        }

        long getNs() { 
            return std::chrono::duration_cast<std::chrono::nanoseconds>(m_end-m_start).count();
        }

        long getMs() { 
            return std::chrono::duration_cast<std::chrono::milliseconds>(m_end-m_start).count();
        }

        bool isRunning() { return m_isRunning; }
};

#endif /* TIMER_H */
