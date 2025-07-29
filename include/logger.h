#pragma once

#include <string>
#include <string_view>
#include <array>
#include <optional>
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
    L_ERROR,
};

template<typename T>
struct EnumStrings;

template<>
struct EnumStrings<LogLevel>
{
    static constexpr std::array<std::pair<LogLevel, std::string_view>, 4> data = {{
        {LogLevel::L_DEBUG, "L_DEBUG"},
        {LogLevel::L_INFO, "L_INFO"},
        {LogLevel::L_WARNING, "L_WARNING"},
        {LogLevel::L_ERROR, "L_ERROR"},
    }};
};

template<typename T>
constexpr std::string_view to_string(T value)
{
    for (auto& [k,v] : EnumStrings<T>::data)
    {
        if (k == value)
        {
            return v;
        }
    }
    return "Unknown";
}

template<typename T>
constexpr std::optional<T> from_string(std::string_view name)
{
    for (auto& [k, v] : EnumStrings<T>::data)
    {
        if (v == name)
        {
            return k;
        }
    }
    return std::nullopt;
}
  
class Logger {
public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger& getInstance();

    void setLevel(LogLevel newLevel);
    LogLevel Logger::getLevel();
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