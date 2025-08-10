#include "display.hpp"
#include "stock.hpp"
#include "alert_manager.hpp"

#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <map>
#include <atomic>
#include <iomanip>
#include <ctime>

extern std::mutex price_mutex;
extern std::map<std::string, std::unique_ptr<Stock>> stockPrices;
extern std::atomic<bool> inWatchMode;
extern std::string currentUser;

void displayLoop() {
    using namespace std::chrono_literals;

    while (inWatchMode) {
        std::this_thread::sleep_for(std::chrono::seconds(10));

        {
            std::lock_guard<std::mutex> lock(price_mutex);
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            char timeBuf[9];
            std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", std::localtime(&now_c));

            for (const auto& [symbol, stock] : stockPrices) {
                if (stock) {
                    std::cout << "[" << timeBuf << "] "
                              << std::setw(6) << std::left << symbol
                              << " = $" << std::fixed << std::setprecision(2)
                              << stock->price << std::endl;
                }
            }
        }

        checkAndTriggerAlerts(currentUser, stockPrices);
    }
}

