#ifndef PORTFOLIO_H
#define PORTFOLIO_H
#define MAX_OWNERS 10

const char* owners[MAX_OWNERS];
int ownerCount = 0;

// ===== Trade / Position Structure =====
struct Trade {
  const char* ticker;   // "AAPL", "QQQM", etc
  float buyPrice;       // price per share
  int quantity; 
  const char* owner = ""; //name of the person who owns the share     
};

// ===== Portfolio Data =====
// Edit this list to update the portfolio

const Trade portfolio[] = {
  //example data: stock name, buy price, quantity, username (optional)
  //{ "AAPL", 150.00, 10 },
  //{ "QQQM", 120.50, 5 },
  //{ "TSLA", 200.25, 2 }

  { "QQQM", 256.88, 1, "Hayden" },
  { "AMZN", 235.80, 1, "Kate" },
  { "AAPL", 233.84, 1, "Kate" },
 // { "SPY", 17.37, 6, "Nathan" },
  { "RPRX", 39.72, 1, "Nathan"}

};



struct ProfitResult {
  float oneDay = 0.0;
  float allTime = 0.0;
};

// ===== Portfolio Size =====
const int PORTFOLIO_SIZE = sizeof(portfolio) / sizeof(portfolio[0]);

void buildOwnerList() {
  ownerCount = 0;

  for (int i = 0; i < PORTFOLIO_SIZE; i++) {
    const char* name = portfolio[i].owner;
    bool exists = false;

    for (int j = 0; j < ownerCount; j++) {
      if (strcmp(owners[j], name) == 0) {
        exists = true;
        break;
      }
    }

    if (!exists && ownerCount < MAX_OWNERS) {
      owners[ownerCount++] = name;
    }
  }
}

bool userOwnsStock(const char* owner, const char* ticker) {
  for (int i = 0; i < PORTFOLIO_SIZE; i++) {
    if (
      strcmp(portfolio[i].owner, owner) == 0 &&
      strcmp(portfolio[i].ticker, ticker) == 0
    ) {
      return true;
    }
  }
  return false;
}

#endif // PORTFOLIO_H
