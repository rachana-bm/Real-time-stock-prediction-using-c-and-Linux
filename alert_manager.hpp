#ifndef ALERT_MANAGER_HPP
#define ALERT_MANAGER_HPP

#include <string>
#include <map>
#include <mutex>
#include <memory>

// Forward declaration
class Stock;

class AlertManager {
public:
    void load(const std::string& user);
    void save(const std::string& user);
    void setAlert(const std::string& user, const std::string& symbol, float threshold);
    void removeAlert(const std::string& user, const std::string& symbol);
    void listAlerts(const std::string& user);
    void checkAndTrigger(const std::string& user, const std::string& symbol, float price);

private:
    std::map<std::string, std::map<std::string, float>> alerts;
    std::mutex alertMutex;

    void ensureDataDirExists();
    void playSound();
};

// Utility for display thread
void checkAndTriggerAlerts(const std::string& user, const std::map<std::string, std::unique_ptr<Stock>>& stockPrices);

#endif

