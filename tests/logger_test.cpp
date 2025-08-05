#include "logger.hpp"
#include <iostream>
#include <chrono>
#include <thread>

using namespace std::chrono_literals;
int main() {
    log_hello(); // Заглушка для проверки
    int x;
    Logger logger("testLog.txt", LogLevel::Info);
    logger.log("First info message", LogLevel::Info);
    std::this_thread::sleep_for(2.0s);
    logger.log("Second info message", LogLevel::Info);
    logger.log("Debug message", LogLevel::Debug);
    std::this_thread::sleep_for(3.0s);
    logger.log("Fatal Error", LogLevel::Error);
    std::cin >> x;
}
