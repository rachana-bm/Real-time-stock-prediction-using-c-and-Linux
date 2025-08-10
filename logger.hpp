#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>
#include <mutex>

class Logger {
public:
    Logger(const std::string& filename);
    void log(const std::string& symbol, float price);

private:
    std::string logFile;
    std::mutex logMutex;
};

#endif // LOGGER_HPP


