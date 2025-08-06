#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>


void log_hello();

enum class LogLevel {
    Debug = 0,
    Info,
    Warning,
    Error
};

enum class LogOutput {
    File,
    Socket,
    Both
};

class Logger {
public:
    Logger(const std::string& filename, LogLevel level = LogLevel::Info,
        LogOutput outputMode = LogOutput::File,
        const std::string& host = "",
        int port = 0);
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

    LogOutput m_outputMode;
    int m_socket = -1;
    sockaddr_in m_serverAddr{};
};
