// stats/main.cpp

#include <iostream>
#include <string>
#include <map>
#include <regex>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <stdexcept>

enum class LogLevel {
    Debug = 0,
    Info,
    Warning,
    Error
};

LogLevel StringToLevel(std::string s){ 
    if (s == "Info") return LogLevel::Info;
    if (s == "Error") return LogLevel::Error;
    if (s == "Debug") return LogLevel::Debug;
    if (s == "Warning") return LogLevel::Warning;
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


// Global statistics
std::map<LogLevel, int> logStats;
int log_total = 0;
int min_len = -1;
int max_len = -1;
int total_len = 0;


bool changed = false;
std::deque<std::chrono::system_clock::time_point> recentLogs;
std::mutex statsMutex;
std::atomic<bool> running(true);
int sinceLastMessage = 0;

int N = 8;
int T = 10;

void removeOldLogs() {
    auto oneHourAgo = std::chrono::system_clock::now() - std::chrono::hours(1);


    // Remove from the front until all old entries are gone
    while (!recentLogs.empty() && recentLogs.front()< oneHourAgo) {
        recentLogs.pop_front();
    }
}


void updateStats(const std::string& line) {
    // Seeking Level label: "[Info]", "[Error]" etc
    std::regex pattern(R"((\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}) \[(Debug|Info|Warning|Error)\])");
    std::smatch match;
    size_t pos = line.find(']')+2;

    if (std::regex_search(line, match, pattern)) {
        changed = true;
        std::cout << line << std::endl;
        std::string datetimeStr = match[1].str();
        std::string level = match[2].str();

        std::lock_guard<std::mutex> lock(statsMutex);
        std::tm tm = {};
        std::istringstream ss(datetimeStr);
        ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
        auto timestamp = std::chrono::system_clock::from_time_t(std::mktime(&tm));
        recentLogs.push_back(timestamp);

        std::string message = line.substr(pos, std::string::npos);
        int len = message.size();
        if (len > 0){
            total_len += len;
            if (min_len < 0){
                min_len = len;
                max_len = len;
            }
            else{
                min_len = std::min(min_len, len);
                max_len = std::max(max_len, len);
            }
        }

        logStats[StringToLevel(level)]++;
        log_total++;
        
        sinceLastMessage++;
    }
}

void printStats(){
    std::lock_guard<std::mutex> lock(statsMutex);
    removeOldLogs();
    std::cout << "\n--- Log statistics ---\n";
    std::cout << "Total Messages: "  << log_total << std::endl;
    for (const auto& [level, count] : logStats) {
        std::cout << LevelToString(level) << ": " << count << std::endl;
    }
    std::cout << "Recent messages: "  << recentLogs.size() << std::endl;
    std::cout << "Largest length: "  << max_len << std::endl;
    std::cout << "Smallest length: "  << min_len << std::endl;
    std::cout << "Average length: " << (int)(total_len/log_total) << std::endl;
    std::cout << "------------------------\n\n";
    changed = false;    
}


// Function: stats output once in T seconds
void printThread() {
    while (running) {
        std::this_thread::sleep_for(std::chrono::seconds(T));
        if (not changed) continue;       
        printStats();       
    }
}


bool isNumber(const std::string& s) {
    if (s.empty()) return false;
    for (char c : s) {
        if (!std::isdigit(c)) return false;
    }
    return true;
}

int safeStoi(const std::string& s, int minVal, int maxVal) {
    if (!isNumber(s)) {
        throw std::invalid_argument(s + "is not a number");
    }
    long long val = std::stoll(s);
    if (val < minVal || val > maxVal) {
        throw std::out_of_range("Value " + std::to_string(val) + " is out of range");
    }
    return static_cast<int>(val);
}

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 9999;
    int N = 5;
    int T = 10;

    try {
        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "--host" && i + 1 < argc) {
                host = argv[++i];
            }
            else if (arg == "--port" && i + 1 < argc) {
                port = safeStoi(argv[++i], 1, 65535);
            }
            else if (arg == "-N" && i + 1 < argc) {
                N = safeStoi(argv[++i], 1, std::numeric_limits<int>::max());
            }
            else if (arg == "-T" && i + 1 < argc) {
                T = safeStoi(argv[++i], 2, 3600);
            }
            else {
                throw std::invalid_argument("Unknown parameter: " + arg);
            }
        }

        // Проверка итоговых значений
        if (T <= 1) throw std::invalid_argument("T must be > 1");
        if (N <= 0) throw std::invalid_argument("N must be > 0");

        std::cout << "Host: " << host << "\n";
        std::cout << "Port: " << port << "\n";
        std::cout << "N: " << N << "\n";
        std::cout << "T: " << T << "\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        std::cerr << "Use examplre:\n"
                  << argv[0] << " --host 127.0.0.1 --port 9999 -N 5 -T 10\n";
        return 1;
    }



    logStats[LogLevel::Info] = 0;
    logStats[LogLevel::Debug] = 0;
    logStats[LogLevel::Warning] = 0;
    logStats[LogLevel::Error] = 0;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr);

    std::cout << "Connecting to " << host << ":" << port << "...\n";
    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to connect server. Make sure that the server is running and available\n";
        close(sock);
        return 1;
    }

    std::cout << "Connected. Start getting logs...\n\n";

    // Run stat output stream
    std::thread statsThread(printThread);

    // Buffer for reading
    char buffer[4096];
    std::string leftover;  // if the connection is broken

    while (running) {
        ssize_t n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) {
            std::cerr << "Connection lost\n";
            break;
        }

        buffer[n] = '\0';
        std::string data = leftover + buffer;

        // Break into lines
        size_t pos = 0;
        std::string line;
        while ((pos = data.find('\n')) != std::string::npos) {
            line = data.substr(0, pos);
            
            if (!line.empty()) {
                updateStats(line);
                if (sinceLastMessage >= N){
                    sinceLastMessage = 0;
                    printStats();
                }
            }
            data.erase(0, pos + 1);
        }
        leftover = data;  // leftover with no \n
    }

    running = false;
    if (statsThread.joinable()) {
        statsThread.join();
    }

    close(sock);
    std::cout << "Stats storing is done.\n";
    return 0;
}