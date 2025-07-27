#include "logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <ctime>
#include <vector>  

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::setLevel(LogLevel newLevel) {
    level = newLevel;
}

void Logger::setLogFile(const std::string& filename) {
    if (logFile.is_open()) {
        logFile.close();
    }
    logFile.open(filename, std::ios_base::app);
    if (!logFile.is_open()) {
        std::cerr << "Error: Could not open log file: " << filename << std::endl;
    }
}

void Logger::log(LogLevel msgLevel, const char* format, ...) {
    if (msgLevel < level) {
        return;
    }

    // Format the variadic arguments
    std::va_list args;
    va_start(args, format);

    // Use vsnprintf to determine the required buffer size
    std::va_list args_copy;
    va_copy(args_copy, args);
    int len = std::vsnprintf(nullptr, 0, format, args_copy);
    va_end(args_copy);

    std::string formatted_message;
    if (len >= 0) {
        std::vector<char> buffer(len + 1);
        std::vsnprintf(buffer.data(), buffer.size(), format, args);
        formatted_message = buffer.data();
    }
    va_end(args);

    // Construct the final log message with timestamp and level
    std::stringstream ss;
    ss << getCurrentTime() << " [" << levelToString(msgLevel) << "] " << formatted_message;

    log_message(ss.str());
}

Logger::Logger() : level(LogLevel::L_DEBUG) {}

Logger::~Logger() {
    if (logFile.is_open()) {
        logFile.close();
    }
}

std::string Logger::getCurrentTime() const {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);

    // std::localtime is not thread-safe, consider alternatives for multi-threaded apps
    std::tm buf;
#ifdef _WIN32
    localtime_s(&buf, &in_time_t);
#else
    localtime_r(&in_time_t, &buf); // POSIX
#endif

    std::stringstream ss;
    ss << std::put_time(&buf, "%Y-%m-%d %X");
    return ss.str();
}

std::string Logger::levelToString(LogLevel msgLevel) const {
    switch (msgLevel) {
    case LogLevel::L_DEBUG:   return "DEBUG";
    case LogLevel::L_INFO:    return "INFO";
    case LogLevel::L_WARNING: return "WARNING";
    case LogLevel::L_ERROR:   return "ERROR";
    default:                  return "UNKNOWN";
    }
}

void Logger::log_message(const std::string& message) {
    std::cout << message << std::endl;

    if (logFile.is_open()) {
        logFile << message << std::endl;
        logFile.flush();
    }
}