#pragma once

#include <string>
#include <fstream>
#include <mutex>

void log_hello();

enum class LogLevel {
    Debug = 0,
    Info,
    Warning,
    Error
};

class Logger {
public:
    Logger(const std::string& filename, LogLevel level = LogLevel::Info);
    ~Logger();

    void log(const std::string& message, LogLevel level = LogLevel::Info);

    void setLevel(LogLevel level);
    LogLevel getLevel() const;
    std::string levelToString(LogLevel level) const;

private:
    std::string getCurrentTimestamp() const;

private:
    std::ofstream m_output;
    LogLevel m_level;
    mutable std::mutex m_mutex;
};
