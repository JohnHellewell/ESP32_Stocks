#ifndef STOCK_H
#define STOCK_H

struct Stock {
    String name;        // Stock symbol, e.g., "AAPL"
    float currentValue; // Current price
    float oneDay;       // Price 24 hours ago
    float oneWeek;      // Price 1 week ago
    float threeMonth;   // Price 3 months ago

    unsigned int quantity;
    float buyPrice;

    // Constructor for convenience
    Stock(const String& stockName = "",
          float current = 0.0,
          float day = 0.0,
          float week = 0.0,
          float threeMonths = 0.0,
          float quan = 0,
          float buyCost = 0.0) 
        : name(stockName), currentValue(current), oneDay(day), oneWeek(week), threeMonth(threeMonths), quantity(quan), buyPrice(buyCost) {}
};

#endif // STOCK_H