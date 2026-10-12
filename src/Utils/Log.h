// This file may contain AI-assisted (Claude Code) code, reviewed by the author.

#pragma once
#include <iostream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#endif

class Log {
private:
    // Constants for ANSI escape codes
    static constexpr const char* ANSI_RED = "\033[1;31m";
    static constexpr const char* ANSI_RESET = "\033[0m";

    // Returns the current system time in [YYYY-MM-DD HH:MM:SS] format
    static std::string getCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm tm{};

#ifdef _WIN32
        localtime_s(&tm, &now_c);
#else
        localtime_r(&now_c, &tm);
#endif
        std::ostringstream oss;
        oss << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] ";
        return oss.str();
    }

public:
    // Enables ANSI support on Windows if needed
    static void init() {
#ifdef _WIN32
        // Enable virtual terminal processing for modern Windows consoles
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = 0;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);
#endif
    }

    // Standard log to console
    template<typename... Args>
    static void log(const Args&... rest) {
        std::cout << getCurrentTimestamp();
        ((std::cout << rest), ...);
        std::cout << std::endl;
    }

    // Error log to console with red color
    template<typename... Args>
    static void error(const Args&... rest) {
        std::cout << ANSI_RED << getCurrentTimestamp();
        ((std::cout << rest), ...);
        std::cout << ANSI_RESET << std::endl;
    }
};

#ifdef DEBUG
#define LOG_INIT() Log::init()
#define LOG(...) Log::log(__VA_ARGS__)
#define LOG_ERROR(...) Log::error(__VA_ARGS__)
#else
#define LOG_INIT()
#define LOG(...) 
#define LOG_ERROR(...) 
#endif