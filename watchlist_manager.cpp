#include "watchlist_manager.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <algorithm>
#include <sstream>

namespace fs = std::filesystem;
using json = nlohmann::json;

WatchlistManager::WatchlistManager() {
    fs::create_directories("data");
}

// 🟢 Load user's watchlist from file
std::vector<std::string> WatchlistManager::getWatchlist(const std::string& username) {
    std::string filename = "data/watchlist_" + username + ".json";
    std::ifstream file(filename);
    if (!file.is_open()) return {};

    json j;
    file >> j;
    if (!j.contains("symbols") || !j["symbols"].is_array()) {
        return {};
    }

    return j["symbols"].get<std::vector<std::string>>();
}

// 🟢 Save updated watchlist
void WatchlistManager::saveWatchlist(const std::string& username, const std::vector<std::string>& symbols) {
    json j;
    j["symbols"] = symbols;

    std::string filename = "data/watchlist_" + username + ".json";
    std::ofstream file(filename);
    file << j.dump(4);
}

// 🟢 Add one or more stocks (comma-separated) to user's watchlist
void WatchlistManager::addStock(const std::string& username, const std::string& symbolInput) {
    std::vector<std::string> current = getWatchlist(username);
    std::vector<std::string> newSymbols = splitSymbols(symbolInput);

    for (const auto& sym : newSymbols) {
        if (std::find(current.begin(), current.end(), sym) == current.end()) {
            current.push_back(sym);
        }
    }

    saveWatchlist(username, current);
    std::cout << "[✅] Added to watchlist.\n";
}

// 🟢 Remove one or more stocks (comma-separated) from user's watchlist
void WatchlistManager::removeStock(const std::string& username, const std::string& symbolInput) {
    std::vector<std::string> current = getWatchlist(username);
    std::vector<std::string> toRemove = splitSymbols(symbolInput);

    current.erase(std::remove_if(current.begin(), current.end(),
        [&toRemove](const std::string& sym) {
            return std::find(toRemove.begin(), toRemove.end(), sym) != toRemove.end();
        }), current.end());

    saveWatchlist(username, current);
    std::cout << "[🗑️] Removed from watchlist.\n";
}

// 🔧 Helper to split comma-separated input and trim
std::vector<std::string> WatchlistManager::splitSymbols(const std::string& input) {
    std::vector<std::string> symbols;
    std::istringstream stream(input);
    std::string token;
    while (std::getline(stream, token, ',')) {
        token.erase(std::remove_if(token.begin(), token.end(), ::isspace), token.end());
        if (!token.empty()) {
            symbols.push_back(token);
        }
    }
    return symbols;
}


