#include "logger.hpp"

#include <iostream>
#include <thread>
#include <string>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include <cctype>

#ifdef _WIN32
#include <windows.h>
void sleep_ms(int ms) { Sleep(ms); }
#else
#include <chrono>
void sleep_ms(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
#endif


LogLevel StringToLevel(std::string s){ 
    std::cout << s;
    if (s == "info") return LogLevel::Info;
    if (s == "error") return LogLevel::Error;
    if (s == "debug") return LogLevel::Debug;
    if (s == "warning") return LogLevel::Warning;
    else throw std::invalid_argument("Only Debug, Info, Warning or Error strings"); 
    //only these strings because we know the log format
}

std::string LevelToString(LogLevel level){ 
    switch (level)
    {
    case LogLevel::Info: return "Info";
    case LogLevel::Debug: return "Debug";
    case LogLevel::Error: return "Error";
    case LogLevel::Warning: return "Warning";
    
    default:
        return "Bad input";
    }
}

// Cuts front and back wspaces
std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) {
        return ""; // only wspaces or empty line
    }
    size_t end = str.find_last_not_of(" \t\n\r\f\v");
    return str.substr(start, end - start + 1);
}

// Every letter lo lowercase
std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

LogOutput StringToOutput(std::string s){ 
    if (s == "file") return LogOutput::File;
    if (s == "socket") return LogOutput::Socket;
    if (s == "both") return LogOutput::Both;
    else throw std::invalid_argument("Only File, Socket or Both strings"); 
}

std::atomic<bool> isRunning(true);


std::pair<std::string, LogLevel> getMessage (std::string& s){
    std::string temp;
    size_t start = 0;
    size_t pos = s.find_first_of('!');
    if (pos == std::string::npos){
        return std::make_pair(s, LogLevel::Default);
    }
    else{
        LogLevel ll;
        temp = s.substr(0, pos);
        temp = trim(toLower(temp));
        if (temp == "info") ll = LogLevel::Info;
        else if (temp == "debug") ll = LogLevel::Debug;
        else if (temp == "error") ll = LogLevel::Error;
        else if (temp == "warning") ll = LogLevel::Warning;
        else {
            ll = LogLevel::Info;
            std::cout << "App: level " << temp << " is not defined" << std::endl;
        }
        //std::cout << pos;
        return std::make_pair(trim(s.substr(pos+1, std::string::npos)), ll);
    }
}

// --- userInputHandler ---

void userInputHandler(Logger& logger) {
    // User message queue
    std::queue<std::pair<std::string, LogLevel>> messageQueue;
    std::mutex queueMutex;
    std::condition_variable hasData;
    std::queue<std::string> to_display;

    // Input stream thread
    auto inputThread = [&]() {
        std::string input;
        std::pair<std::string, LogLevel> output;
        while (isRunning) {
            
            std::cout << "> ";
            std::getline(std::cin, input);
            output = getMessage(input);
            if (!input.empty()) {
                std::lock_guard<std::mutex> lock(queueMutex);
                messageQueue.push(output);
                hasData.notify_one();
            }            
        }
        hasData.notify_all(); // wake logger to terminate
    };

    // Dumping stream thread
    auto loggingThread = [&]() {
        while (isRunning) {
            std::unique_lock<std::mutex> lock(queueMutex);
            hasData.wait(lock, [&]() { return !messageQueue.empty() || !isRunning; });

            if (!messageQueue.empty()) {
                std::pair<std::string, LogLevel> msg = messageQueue.front();
                messageQueue.pop();
                lock.unlock(); // free mutex before calling log()
                if(msg.second == LogLevel::Default) msg.second = logger.getDefaultLevel();
                if (msg.first == "exit") isRunning = false;
                else if (msg.first == "chlevel"){ 
                    logger.setLevel(msg.second);
                    std::cout << "Minimum level changed to " + logger.levelToString(msg.second) << std::endl;
                }
                else if (msg.first == "chdefault"){ 
                    logger.setDefaultLevel(msg.second);
                    std::cout << "Default level changed to " + logger.levelToString(msg.second) << std::endl;
                }
                else { logger.log(msg.first, msg.second);  }
                
            }
        }
    };

    // Run both threads
    std::thread inputT(inputThread);
    std::thread loggingT(loggingThread);

    // Wait termination
    inputT.join();
    loggingT.join();
}

int main(int argc, char* argv[]) {
    std::string mode = "both";
    std::string filePath = "log.txt";
    std::string level = "info";
    std::string host = "127.0.0.1";
    int port = 9999;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "--mode" && i + 1 < argc) {
            mode = argv[++i];
        }
        else if (arg == "--file" && i + 1 < argc) {
            filePath = argv[++i];
        }
        else if (arg == "--level" && i + 1 < argc) {
            level = argv[++i];
        }
        else {
            std::cerr << "Неизвестный параметр: " << arg << "\n";
        }
    }
    
    Logger logger(filePath, StringToLevel(trim(toLower(level))), StringToOutput(trim(toLower(mode))), host, port);

    std::thread input(userInputHandler, std::ref(logger));

    input.join();

    std::cout << "App terminated.\n";
    return 0;
}