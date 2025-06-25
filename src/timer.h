// 
// timer.h
// ConwaysGameOfLife
// 
// Noah Hitz 2025
// 

#ifndef TIMER_H
#define TIMER_H

#include <chrono>
#include <vector>

class Timer {
    private:
        std::chrono::time_point<std::chrono::high_resolution_clock> m_start;
        std::chrono::time_point<std::chrono::high_resolution_clock> m_end;
        bool m_isRunning = false;

        int index = 0;
        std::vector<long> intervals;
        bool filled = false;
        
    public:
        Timer() { 
            intervals = std::vector<long>(1);
        }
        
        Timer(int average)  {
            intervals = std::vector<long>(average);
        }

        void start() {
            m_start = std::chrono::high_resolution_clock::now();
            m_isRunning = true;
        }

        void stop() { 
            m_end = std::chrono::high_resolution_clock::now();
            if(!filled && index == intervals.size()-1) filled = true;
            intervals[(index++)%intervals.size()] = getNs();
            m_isRunning = false;
        }

        void resume() {
            m_isRunning = true;
        }

        double getAverageNs() {
            double averageInterval = 0;
            int size = filled ? intervals.size() : index;
            for(int i = 0; i < size; i++)
                averageInterval += (double) intervals[i]/size;
            return averageInterval;
        }
        
        double getAverageMs() {
            return getAverageNs()/1'000'000;
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
