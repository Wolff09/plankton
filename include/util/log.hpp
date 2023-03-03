#pragma once
#ifndef PLANKTON_UTIL_LOG_HPP
#define PLANKTON_UTIL_LOG_HPP

#include <array>
#include <vector>
#include <iostream>

namespace plankton {
    
    // void LogMsg(const std::string& msg, std::size_t level = 0) { /* ... */ }
    // #define LOG(...) LogMsg(__VA_ARGS__)

    struct LogStream {
        static std::ostream& Out() {
            return Instance().out;
        }
        static std::ostream& Err() {
            return Instance().err;
        }
        static void SetOut(std::ostream& stream) {
            Instance().out = stream;
        }
        static void SetErr(std::ostream& stream) {
            Instance().err = stream;
        }
        private:
            std::reference_wrapper<std::ostream> out;
            std::reference_wrapper<std::ostream> err;
            explicit LogStream(std::ostream& out, std::ostream& err) : out(out), err(err) {}
            static LogStream& Instance() {
                static LogStream singleton(std::cout, std::cerr);
                return singleton;
            }
    };

    #ifdef ENABLE_DEBUG_PRINTING
        #define DEBUG(X) { LogStream::Out() << X; }
        #define DEBUG_FOREACH(X, F) { for (const auto& elem : X) { F(elem); } }
    #else
        #define DEBUG(X) {}
        #define DEBUG_FOREACH(X, F) {}
    #endif
    
    #define INFO(X) { LogStream::Out() << X; }

    #define WARNING(X) { LogStream::Err() << "WARNING: " << X; }
    
    #define ERROR(X) { LogStream::Err() << "ERROR: " << X; }

    struct StatusStack {
        inline void Push(std::string string) { stack.push_back(std::move(string)); }
        inline void Push(std::string string, std::string other) { stack.push_back(std::move(string) + std::move(other)); }
        inline void Push(std::string string, std::size_t other) { stack.push_back(std::move(string) + std::to_string(other)); }
        inline void Pop() { stack.pop_back(); }
        inline void Print(std::ostream& stream) const {
            for (const auto& elem : stack) stream << "[" << elem << "]";
            if (!stack.empty()) stream << " ";
        }
    private:
        std::vector<std::string> stack;
    };

    inline std::ostream& operator<<(std::ostream& stream, const StatusStack& statusStack) {
        statusStack.Print(stream);
        return stream;
    }

} // namespace plankton

#endif //PLANKTON_UTIL_LOG_HPP