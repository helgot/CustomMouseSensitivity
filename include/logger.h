#pragma once

#include <string>
#include <sstream>
#include <fstream>
#include <cstdarg>


#define LOG_DEBUG(...) Logger::getInstance().log(LogLevel::L_DEBUG, __VA_ARGS__)
#define LOG_INFO(...) Logger::getInstance().log(LogLevel::L_INFO, __VA_ARGS__)
#define LOG_WARNING(...) Logger::getInstance().log(LogLevel::L_WARNING, __VA_ARGS__)
#define LOG_ERROR(...) Logger::getInstance().log(LogLevel::L_ERROR, __VA_ARGS__)


// Enum for different log levels
enum class LogLevel {
    L_DEBUG,
    L_INFO,
    L_WARNING,
    L_ERROR
};

class Logger {
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger& getInstance();

    void setLevel(LogLevel newLevel);
    void setLogFile(const std::string& filename);
    void log(LogLevel msgLevel, const char* format, ...);

private:
    Logger();
    ~Logger();

private:
    std::string getCurrentTime() const;
    std::string levelToString(LogLevel level) const;
    void log_message(const std::string& message);

    LogLevel level;
    std::ofstream logFile;
};