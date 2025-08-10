#ifndef STOCK_HPP
#define STOCK_HPP

#include <string>

struct Stock {
    std::string symbol;
    float price;

    Stock(const std::string& sym, float p) : symbol(sym), price(p) {}
};

#endif

