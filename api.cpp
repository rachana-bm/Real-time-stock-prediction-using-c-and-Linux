#include "api.hpp"
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <iostream>

using json = nlohmann::json;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}

std::string fetchStockPrice(const std::string& symbol) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    std::string apiKey = "5e198de560db4a6a83090f9b314d6524";  // Replace this
    std::string url = "https://api.twelvedata.com/price?symbol=" + symbol + "&apikey=" + apiKey;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "[CURL] Failed: " << curl_easy_strerror(res) << "\n";
            return "0";
        }
        curl_easy_cleanup(curl);
    }

    try {
        auto j = json::parse(readBuffer);
        if (j.contains("price")) {
            return j["price"];
        } else {
            std::cerr << "[ERROR] JSON missing expected field: " << j.dump() << "\n";
            return "0";
        }
    } catch (...) {
        std::cerr << "[ERROR] Failed to parse price JSON.\n";
        return "0";
    }
}

