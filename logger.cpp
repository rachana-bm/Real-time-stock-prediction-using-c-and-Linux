#include "logger.hpp"
#include <fstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

Logger::Logger(const std::string& filename) : logFile(filename) {
    fs::create_directories("data");

    std::ofstream file(logFile, std::ios::app);
    if (!file.is_open()) {
        std::cerr << "[LOGGER] ❌ Could not open log file.\n";
        return;
    }

    if (file.tellp() == 0) {
        file << "Timestamp,Symbol,Price\n";
    }
}

void Logger::log(const std::string& symbol, float price) {
    std::lock_guard<std::mutex> lock(logMutex);

    std::ofstream file(logFile, std::ios::app);
    if (!file.is_open()) {
        std::cerr << "[LOGGER] ❌ Cannot write to log file.\n";
        return;
    }

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* timeInfo = std::localtime(&now_c);

    std::ostringstream timestamp;
    timestamp << std::put_time(timeInfo, "%Y-%m-%d %H:%M:%S");

    file << timestamp.str() << "," << symbol << "," << price << "\n";
    file.flush();
}

