#include "logger.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>
#include <ctime>

Logger::Logger(const std::string& filename, LogLevel level)
    : m_level(level)
{
    m_output.open(filename, std::ios::app);
    // �� ��: ��� ���������� � ������� ����� �������� ��������� ����� �������
    if (!m_output.is_open()) {
        // ������ �� ������ � ����������� ������ "�� ����� ��������"
        // ����� �������� ���� ������, ���� �����������
    }
}

Logger::~Logger() {
    if (m_output.is_open()) {
        m_output.close();
    }
}

void Logger::log(const std::string& message, LogLevel level) {
    if (level < m_level || !m_output.is_open()) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    m_output << "[" << getCurrentTimestamp() << "] "
        << "[" << levelToString(level) << "] "
        << message << std::endl;
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
