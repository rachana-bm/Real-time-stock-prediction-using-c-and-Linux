#include "api.hpp"
#include "display.hpp"
#include "stock.hpp"
#include "logger.hpp"
#include "watchlist_manager.hpp"
#include "alert_manager.hpp"

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <memory>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <fstream>

std::mutex price_mutex;
std::map<std::string, std::unique_ptr<Stock>> stockPrices;
std::shared_ptr<Logger> logger = std::make_shared<Logger>("data/stock_log.csv");
WatchlistManager manager;
std::unique_ptr<AlertManager> alertManager;

std::atomic<bool> inWatchMode(false);
std::atomic<bool> inLookupMode(false);
std::string currentUser;

std::condition_variable_any cv;
std::mutex cv_mtx;

std::string trim(const std::string& s) {
    auto a = s.find_first_not_of(" \t\r\n");
    auto b = s.find_last_not_of(" \t\r\n");
    return (a == std::string::npos ? "" : s.substr(a, b - a + 1));
}

std::vector<std::string> parseSymbols(const std::string& input) {
    std::vector<std::string> syms;
    std::istringstream ss(input);
    std::string tok;
    while (std::getline(ss, tok, ',')) {
        tok = trim(tok);
        if (!tok.empty())
            syms.push_back(tok);
    }
    return syms;
}

void fetchLoop(const std::string& symbol, bool logToCSV) {
    while (inWatchMode || inLookupMode) {
        try {
            float price = std::stof(fetchStockPrice(symbol));
            {
                std::lock_guard<std::mutex> lk(price_mutex);
                if (!stockPrices[symbol])
                    stockPrices[symbol] = std::make_unique<Stock>(symbol, price);
                else
                    stockPrices[symbol]->price = price;
            }

            if (logToCSV)
                logger->log(symbol, price);
            else
                std::cout << "[LOOKUP] " << symbol << " = $" << price << "\n";

            if (alertManager)
                alertManager->checkAndTrigger(currentUser, symbol, price);

        } catch (...) {
            std::cerr << "[⚠️] Error parsing price for " << symbol << "\n";
        }

        std::unique_lock<std::mutex> lk(cv_mtx);
        cv.wait_for(lk, std::chrono::seconds(10), [] {
            return !(inWatchMode || inLookupMode);
        });
    }
}

void runLookupMode(const std::vector<std::string>& symbols) {
    std::cout << "[🔍] Lookup Mode started — press ENTER to return.\n";
    inLookupMode = true;

    std::vector<std::thread> threads;
    for (auto& s : symbols)
        threads.emplace_back(fetchLoop, s, false);
    threads.emplace_back(displayLoop);

    std::cin.get(); // Wait for ENTER

    inLookupMode = false;
    cv.notify_all();
    for (auto& t : threads) t.join();
    std::cout << "[✅] Lookup Mode ended.\n";
}

void runWatchMode() {
    auto list = manager.getWatchlist(currentUser);
    if (list.empty()) {
        std::cout << "[ℹ️] Watchlist is empty.\n";
        return;
    }

    stockPrices.clear();
    inWatchMode = true;

    std::vector<std::thread> threads;
    for (auto& s : list)
        threads.emplace_back(fetchLoop, s, true);
    threads.emplace_back(displayLoop);

    std::cout << "[👀] Watch mode started — press ENTER to stop.\n";
    std::cin.get(); // Wait for ENTER

    inWatchMode = false;
    cv.notify_all();
    for (auto& t : threads) t.join();
    std::cout << "[✅] Watch Mode ended.\n";
}

void showNasdaqSymbols() {
    std::ifstream file("nasdaq_symbols.txt");
    if (!file.is_open()) {
        std::cerr << "[❌] Could not open nasdaq_symbols.txt\n";
        return;
    }

    std::string line;
    std::cout << "\n📄 [NASDAQ Symbols and Companies List]\n";

    while (std::getline(file, line)) {
        std::cout << line << "\n";
    }

    std::cout << "\n[ℹ️] Press ENTER to return.\n";
    std::cin.get(); // Wait for ENTER
}

void modeLoop() {
    while (true) {
        std::string mode;
        std::cout << "\nMode (watch/lookup/symbols/exit): ";
        std::getline(std::cin, mode);

        if (mode == "exit") {
            break;
        } else if (mode == "symbols") {
            showNasdaqSymbols();
        } else if (mode == "lookup") {
            std::string inp;
            std::cout << "Enter stock symbols (comma-separated): ";
            std::getline(std::cin, inp);
            runLookupMode(parseSymbols(inp));
        } else if (mode == "watch") {
            std::cout << "Enter username: ";
            std::getline(std::cin, currentUser);
            alertManager = std::make_unique<AlertManager>();
            alertManager->load(currentUser);

            std::string cmd;
            while (true) {
                std::cout << "\n[Commands: add <symbol>, remove <symbol>, alert <symbol> <price>, remove_alert <symbol>, list_alerts, watch, exit]\n> ";
                std::getline(std::cin, cmd);
                cmd = trim(cmd);

                if      (cmd.rfind("add ", 0) == 0) {
                    for (auto& s : parseSymbols(cmd.substr(4)))
                        manager.addStock(currentUser, s);
                } else if (cmd.rfind("remove ", 0) == 0) {
                    for (auto& s : parseSymbols(cmd.substr(7)))
                        manager.removeStock(currentUser, s);
                } else if (cmd.rfind("alert ", 0) == 0) {
                    std::istringstream ss(cmd.substr(6));
                    std::string s; float p;
                    if (ss >> s >> p) alertManager->setAlert(currentUser, s, p);
                    else std::cout << "[❌] Usage: alert <symbol> <price>\n";
                } else if (cmd.rfind("remove_alert ", 0) == 0) {
                    auto s = trim(cmd.substr(13));
                    if (s.empty()) std::cout << "[❌] Usage: remove_alert <symbol>\n";
                    else alertManager->removeAlert(currentUser, s);
                } else if (cmd == "list_alerts") {
                    alertManager->listAlerts(currentUser);
                } else if (cmd == "watch") {
                    runWatchMode();
                } else if (cmd == "exit") {
                    break;
                } else {
                    std::cout << "[❓] Unknown command.\n";
                }
            }
        } else {
            std::cout << "[❌] Invalid mode. Type 'watch', 'lookup', 'symbols', or 'exit'.\n";
        }
    }
}

int main() {
    modeLoop();
    std::cout << "[👋] Exiting program.\n";
    return 0;
}

