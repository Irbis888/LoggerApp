#include "logger.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <ctime>

#include <stdexcept>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cerrno>


Logger::Logger(const std::string& filename, LogLevel level,
    LogOutput outputMode,
    const std::string& host,
    int port)
    : m_level(level), m_outputMode(outputMode)
{
    m_output.open(filename, std::ios::app);
    if (!m_output.is_open()) {
        std::cout << "File does not exist" << std::endl;
    }
    if (m_outputMode == LogOutput::Socket || m_outputMode == LogOutput::Both) {
        m_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (m_socket == -1) {
            throw std::runtime_error("Failed to create socket");
        }

        m_serverAddr.sin_family = AF_INET;
        m_serverAddr.sin_port = htons(port);
        if (inet_pton(AF_INET, host.c_str(), &m_serverAddr.sin_addr) <= 0) {
            throw std::invalid_argument("Invalid IP address: '" + host + "'");
        }

        if (connect(m_socket, (sockaddr*)&m_serverAddr, sizeof(m_serverAddr)) < 0) { 
            perror("connect failed");
            throw std::runtime_error("Failed to connect to server");
        }
    }
}

Logger::~Logger() {
    if (m_output.is_open()) {
        m_output.close();
    }
    if (m_socket != -1) {
        close(m_socket);
    }
}

void Logger::log(const std::string& message, LogLevel level) {
    if (level < m_level) return;

    std::string timestamped = getCurrentTimestamp() + " [" + levelToString(level) + "] " + message + "\n";

    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_outputMode == LogOutput::File || m_outputMode == LogOutput::Both) {
        m_output << timestamped;
        m_output.flush();
    }

    if (m_outputMode == LogOutput::Socket || m_outputMode == LogOutput::Both) {
        if (m_socket != -1) {
            send(m_socket, timestamped.c_str(), timestamped.size(), 0);
        }
    }
}

void Logger::setLevel(LogLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_level = level;
}

LogLevel Logger::getLevel() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_level;
}

std::string Logger::levelToString(LogLevel level) const {
    switch (level) {
    case LogLevel::Debug:   return "Debug";
    case LogLevel::Info:    return "Info";
    case LogLevel::Warning: return "Warning";
    case LogLevel::Error:   return "Error";
    default:                return "Unknown";
    }
}

std::string Logger::getCurrentTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto t = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);  // Windows
#else
    localtime_r(&t, &tm);  // POSIX
#endif
    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return buffer;
}


void log_hello() {
    std::cout << "Logger initialized!\n";
}
