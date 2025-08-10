#ifndef WATCHLIST_MANAGER_HPP
#define WATCHLIST_MANAGER_HPP

#include <string>
#include <vector>

class WatchlistManager {
public:
    WatchlistManager();

    // Get user's current watchlist
    std::vector<std::string> getWatchlist(const std::string& username);

    // Add one or more (comma-separated) stocks to watchlist
    void addStock(const std::string& username, const std::string& symbolInput);

    // Remove one or more (comma-separated) stocks from watchlist
    void removeStock(const std::string& username, const std::string& symbolInput);

private:
    // Helper to split comma-separated input and trim
    std::vector<std::string> splitSymbols(const std::string& input);

    // Save the updated watchlist to file
    void saveWatchlist(const std::string& username, const std::vector<std::string>& symbols);
};

#endif  // WATCHLIST_MANAGER_HPP

