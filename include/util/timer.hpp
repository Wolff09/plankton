#pragma once
#ifndef PLANKTON_UTIL_TIMER_HPP
#define PLANKTON_UTIL_TIMER_HPP

#include <chrono>
#include <sstream>
#include "log.hpp"

namespace plankton {
    
    class Timer {
    private:
        std::string info;
        std::size_t counter;
        std::chrono::milliseconds elapsed;

        [[nodiscard]] inline std::string ToString(const std::string& note, bool sortable = false) const {
            std::stringstream stream;
            auto milli = std::to_string(elapsed.count());
            if (sortable) stream << "[" << std::string(10 - milli.length(), '0') << milli << "ms] ";
            stream << note << " '" << info << "' (" << counter << "): ";
            stream << milli << "ms";
            stream << std::endl;
            return stream.str();
        }

    public:
        class Measurement {
        private:
            Timer& parent;
            std::chrono::time_point<std::chrono::steady_clock> start;

        public:
            Measurement(const Measurement& other) = delete;
            explicit Measurement(Timer& parent) : parent(parent), start(std::chrono::steady_clock::now()) {}

            ~Measurement() {
                auto end = std::chrono::steady_clock::now();
                auto myElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
                parent.elapsed += myElapsed;
                parent.counter++;
            }
        };

        explicit Timer(std::string info) : info(std::move(info)), counter(0), elapsed(0) {}
        ~Timer() {} //{ INFO(ToString("Total time measured for", true)) }
        inline void Print() const {} //{ INFO(ToString("Time measured for")) }
        inline Measurement Measure() { return Measurement(*this); }

    private:
        std::optional<Measurement> latest;

    public:
        inline void Start() { assert(!latest.has_value()); latest.emplace(*this); }
        inline void Stop() { assert(latest.has_value()); latest.reset(); }
    };


    //#ifdef ENABLE_TIMER
    //    #define MEASURE(X) static Timer timer(X); auto measurement = timer.Measure();
    //#else
        #define MEASURE(X) {}
    //#endif

} // plankton

#endif //PLANKTON_UTIL_TIMER_HPP