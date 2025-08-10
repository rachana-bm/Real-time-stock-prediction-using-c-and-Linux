#include "alert_manager.hpp"
#include "stock.hpp"

#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <unistd.h>
#include <sys/stat.h>
#include <cstdlib>

extern std::mutex price_mutex;
extern std::map<std::string, std::unique_ptr<Stock>> stockPrices;

using json = nlohmann::json;

void AlertManager::ensureDataDirExists() {
    struct stat st{};
    if (stat("data", &st) == -1) {
        mkdir("data", 0755);
    }
}

void AlertManager::load(const std::string& user) {
    std::lock_guard<std::mutex> lock(alertMutex);
    alerts[user].clear();

    std::ifstream file("data/alerts_" + user + ".json");
    if (!file.is_open()) return;

    try {
        json j;
        file >> j;
        for (auto& [symbol, threshold] : j.items()) {
            alerts[user][symbol] = threshold;
        }
    } catch (...) {
        std::cerr << "[⚠️] Failed to parse alerts file." << std::endl;
    }
}

void AlertManager::save(const std::string& user) {
    json j;

    {
        std::lock_guard<std::mutex> lock(alertMutex);
        for (const auto& [symbol, threshold] : alerts[user])
            j[symbol] = threshold;
    }

    ensureDataDirExists();
    std::ofstream file("data/alerts_" + user + ".json");
    if (!file.is_open()) {
        std::cerr << "[⚠️] Failed to write alert file." << std::endl;
        return;
    }

    file << j.dump(4);
}

void AlertManager::setAlert(const std::string& user, const std::string& symbol, float threshold) {
    {
        std::lock_guard<std::mutex> lock(alertMutex);
        alerts[user][symbol] = threshold;
    }
    save(user);
    std::cout << "[🔔] Alert set for " << symbol << " at $" << threshold << std::endl;
}

void AlertManager::removeAlert(const std::string& user, const std::string& symbol) {
    bool removed = false;

    {
        std::lock_guard<std::mutex> lock(alertMutex);
        removed = alerts[user].erase(symbol);
    }

    if (removed) {
        save(user);
        std::cout << "[❌] Alert removed for " << symbol << std::endl;
    } else {
        std::cout << "[ℹ️] No alert found for " << symbol << std::endl;
    }
}

void AlertManager::listAlerts(const std::string& user) {
    std::lock_guard<std::mutex> lock(alertMutex);
    if (alerts[user].empty()) {
        std::cout << "[ℹ️] No alerts set." << std::endl;
        return;
    }

    std::cout << "\n[📢] Alerts for " << user << ":" << std::endl;
    for (const auto& [symbol, threshold] : alerts[user]) {
        std::cout << "  - " << symbol << " crosses $" << threshold << std::endl;
    }
}

void AlertManager::checkAndTrigger(const std::string& user, const std::string& symbol, float currentPrice) {
    static std::map<std::string, float> lastSeenPrices;
    float threshold = 0;
    bool triggered = false;

    {
        std::lock_guard<std::mutex> lock(alertMutex);
        auto it = alerts[user].find(symbol);
        if (it == alerts[user].end()) return;

        threshold = it->second;

        float prevPrice = lastSeenPrices.count(symbol) ? lastSeenPrices[symbol] : currentPrice;
        lastSeenPrices[symbol] = currentPrice;

        // Check crossing over the threshold in either direction
        if ((prevPrice < threshold && currentPrice >= threshold) ||
            (prevPrice > threshold && currentPrice <= threshold)) {
            alerts[user].erase(it);
            triggered = true;
        }
    }

    if (triggered) {
        std::cout << "[🚨] ALERT: " << symbol << " = $" << currentPrice
                  << " (crossed $" << threshold << ")" << std::endl;
        playSound();
        save(user);
    }
}

void checkAndTriggerAlerts(const std::string& user, const std::map<std::string, std::unique_ptr<Stock>>& stockPrices) {
    AlertManager manager;
    manager.load(user);

    for (const auto& [symbol, stock] : stockPrices) {
        if (stock) {
            manager.checkAndTrigger(user, symbol, stock->price);
        }
    }
}

void AlertManager::playSound() {
    const char* path = "sound/alert.wav";
    if (access(path, F_OK) == 0) {
        std::cout << "[🔊] Playing sound alert!" << std::endl;
        system(("aplay -q " + std::string(path) + " &").c_str());
    } else {
        std::cout << "[❌] " << path << " not found!" << std::endl;
    }
}

