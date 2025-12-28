#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <WiFiClientSecure.h>
#include <U8g2lib.h>
#include <Wire.h>

#include "secrets.h"
#include "stock.h"
#include "portfolio.h"
#include "timeOfDay.h"


// -------------------- Stock Symbols --------------------
String stockSymbols[] = {
  "QQQM",
  "AAPL",
  "AMZN",
  "MSFT",
  "GOOGL",
  "TSLA"
};

enum TextAlign {
  ALIGN_LEFT,
  ALIGN_CENTER,
  ALIGN_RIGHT
};

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 22, 21);
#define X_OFFSET 0

const int numStocks = sizeof(stockSymbols) / sizeof(stockSymbols[0]);
Stock stocks[numStocks];

// -------------------- Display --------------------
#define SCREEN_WIDTH 128
#define RIGHT_MARGIN 2

int getAlignedX(const char* text, TextAlign align) {
  uint16_t textWidth = u8g2.getStrWidth(text);

  switch (align) {
    case ALIGN_CENTER:
      return (SCREEN_WIDTH - textWidth) / 2;

    case ALIGN_RIGHT:
      return SCREEN_WIDTH - textWidth - RIGHT_MARGIN;

    case ALIGN_LEFT:
    default:
      return 0;
  }
}

const char* floatToString(float value, int width = 6, int precision = 2) {
  static char buffer[16];  // static so it persists after function ends
  dtostrf(value, width, precision, buffer);
  return buffer;
}

String addSymbols(float amount){
  String temp = "$";
  if(amount > 0.0){
    temp += "+";
  }
  temp += floatToString(amount);
  return temp;
}

void textCard(const char *line1 = "",
              const char *line2 = "",
              const char *line3 = "",
              TextAlign align1 = ALIGN_LEFT,
              TextAlign align2 = ALIGN_LEFT,
              TextAlign align3 = ALIGN_LEFT) {

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x13_tf);

  if (*line1)
    u8g2.drawStr(getAlignedX(line1, align1), 10, line1);

  if (*line2)
    u8g2.drawStr(getAlignedX(line2, align2), 30, line2);

  if (*line3)
    u8g2.drawStr(getAlignedX(line3, align3), 50, line3);

  u8g2.sendBuffer();
}


void profitsCard(const char *owner, const char *name, float oneDay, float allTime) {
  u8g2.clearBuffer(); // Clear buffer
  u8g2.setFont(u8g2_font_7x13_tf);
  u8g2.drawStr(X_OFFSET, 10, name);
  u8g2.drawStr(getAlignedX(owner, ALIGN_RIGHT), 10, owner);
  u8g2.drawStr(X_OFFSET, 30, "24H" );
  String temp = addSymbols(oneDay);
  u8g2.drawStr(getAlignedX(temp.c_str(), ALIGN_RIGHT), 30, temp.c_str());
  u8g2.drawStr(X_OFFSET, 55, "All Time");
  temp = addSymbols(allTime);
  u8g2.drawStr(getAlignedX(temp.c_str(), ALIGN_RIGHT), 55, temp.c_str());
  u8g2.drawHLine(0, 37, 140);
  u8g2.sendBuffer(); // Send buffer to display
}

void stockDisplay(const char *name, float currentPrice, float oneDayPrice) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_7x13_tf);

  u8g2.drawStr(getAlignedX(getGreeting(), ALIGN_CENTER), 10, getGreeting());
  u8g2.drawStr(X_OFFSET, 30, name);

  String priceStr = "$" + String(floatToString(currentPrice));
  u8g2.drawStr(getAlignedX(priceStr.c_str(), ALIGN_RIGHT), 30, priceStr.c_str());

  float oneDayDifference = currentPrice - oneDayPrice;
  String diffStr = "$" + String(floatToString(oneDayDifference));
  u8g2.drawStr(getAlignedX(diffStr.c_str(), ALIGN_RIGHT), 55, diffStr.c_str());

  u8g2.drawStr(X_OFFSET, 55, "24H");
  u8g2.drawHLine(0, 37, 140);
  u8g2.sendBuffer();
}

void timeCard(){
  textCard("New York City", getESTMilitaryTime(), getMarketStatus(), ALIGN_CENTER, ALIGN_CENTER, ALIGN_CENTER);
}


// -------------------- WiFi --------------------
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  textCard("Joining WiFi...", WIFI_SSID, "", ALIGN_CENTER, ALIGN_CENTER);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());
  String ipStr = WiFi.localIP().toString();
  textCard("Connected!", "IPV4 address:", ipStr.c_str());
  delay(1000);

  // NTP for timestamps (optional)
  textCard("Sync time & date", "pool.ntp.org", "wait response...", ALIGN_LEFT, ALIGN_CENTER);
  Serial.println("Starting NTP sync...");
  configTzTime(TZ_NEW_YORK, "pool.ntp.org", "time.nist.gov");

  time_t now = 0;
  unsigned long startAttempt = millis();
  while (now < 100000 && millis() - startAttempt < 10000) {
    now = time(nullptr);
    Serial.print("Waiting for NTP... epoch=");
    Serial.println(now);
    delay(500);
  }

  if (now < 100000) {
    Serial.println("NTP sync FAILED");
    textCard("NTP sync FAILED", "", "restarting");
    delay(1000);
    ESP.restart();
  } else {
    Serial.println("NTP sync successful");
    textCard("NTP sync complete");
    delay(500);
  }
}

// -------------------- HTTP + JSON --------------------
bool fetchJSON(const String& url, JsonDocument& doc) {
  WiFiClientSecure client;
  client.setInsecure();  // for dev only

  HTTPClient https;
  https.setReuse(false);

  if (!https.begin(client, url)) {
    Serial.println("❌ HTTPS begin failed");
    return false;
  }

  int httpCode = https.GET();
  if (httpCode != HTTP_CODE_OK) {
    Serial.print("❌ HTTP error: ");
    Serial.println(httpCode);
    https.end();
    client.stop();
    return false;
  }

  DeserializationError err = deserializeJson(doc, https.getStream());
  https.end();
  client.stop();

  if (err) {
    Serial.print("❌ JSON parse error: ");
    Serial.println(err.c_str());
    return false;
  }

  return true;
}

int getStockIndex(String name){
  for(int i=0; i<numStocks; i++){
    if(stocks[i].name == name){
      return i;
    }
  }
  return -1;
}



// -------------------- Stock Updates --------------------
// Finnhub: current price + 24h prev close
void updateStockCurrent(Stock &stock, bool showDisplay = false) {
  if(showDisplay){
    textCard("Fetching data...", stock.name.c_str(), "", ALIGN_CENTER, ALIGN_CENTER);
  }

  String url = "https://finnhub.io/api/v1/quote?symbol=" + stock.name + "&token=" + FINNHUB_API_KEY;

  StaticJsonDocument<256> doc;
  if (fetchJSON(url, doc)) {
    stock.currentValue = doc["c"] | 0.0;
    stock.oneDay       = doc["pc"] | 0.0;

    Serial.print(stock.name);
    Serial.print(" current: ");
    Serial.println(stock.currentValue);

    Serial.print(stock.name);
    Serial.print(" 24h ago: ");
    Serial.println(stock.oneDay);
  } else {
    Serial.print("❌ Failed to fetch current price for ");
    Serial.println(stock.name);
  }
}

// -------------------- Initialization --------------------
void createStructs() {
  for (int i = 0; i < numStocks; i++) {
    stocks[i].name = stockSymbols[i];
  }
}
/*
void loadPortfolio(){
  for(int i=0; i<PORTFOLIO_SIZE; i++){
    if(portfolio[i].quantity > 0){
      int index = getStockIndex(portfolio[i].ticker);
      if(index != -1){
        float sum = (stocks[index].quantity * stocks[index].buyPrice) + (portfolio[i].buyPrice * portfolio[i].quantity);
        int newQuantity = stocks[index].quantity + portfolio[i].quantity;
        float avgPrice = round(sum * 100.0) / 100.0; //two decimal places
        stocks[index].quantity = newQuantity;
        stocks[index].buyPrice = avgPrice;
        Serial.print("Updated portfolio of ");
        Serial.print(stocks[index].name);
        Serial.print(" with ");
        Serial.print(newQuantity);
        Serial.print(" shares at ");
        Serial.print(avgPrice);
        Serial.println(" each");
      } else {
        Serial.print("Missing stock: Could not find stock by the name of \"");
        Serial.print(portfolio[i].ticker);
        Serial.println("\", skipped. Add to list insode Stocks.ino");
      }
    }
  }
}
*/
ProfitResult getUserStockProfit(const char* owner, const char* ticker, const Stock stocks[], int stockCount) {
  ProfitResult result;

  const Stock* stock = findStock(ticker, stocks, stockCount);
  if (!stock) return result;

  for (int i = 0; i < PORTFOLIO_SIZE; i++) {
    const Trade& t = portfolio[i];

    if (
      strcmp(t.owner, owner) == 0 &&
      strcmp(t.ticker, ticker) == 0
    ) {
      result.oneDay += (stock->currentValue - stock->oneDay) * t.quantity;
      result.allTime += (stock->currentValue - t.buyPrice) * t.quantity;
    }
  }

  return result;
}




// -------------------- Background Task --------------------
const unsigned long UPDATE_INTERVAL_CURRENT = 60000; // 1 min

void stockUpdateTask(void *parameter) {
  for (;;) { // infinite loop for FreeRTOS task
    for (int i = 0; i < numStocks; i++) {
      updateStockCurrent(stocks[i], false);
      vTaskDelay(500 / portTICK_PERIOD_MS); // small delay between stocks
    }
    vTaskDelay(UPDATE_INTERVAL_CURRENT / portTICK_PERIOD_MS);
  }
}

// -------------------- ESP32 --------------------
void setup() {
  Serial.begin(115200);
  u8g2.begin(); 
  delay(100);

  connectToWiFi();

  textCard("Initializing", "stocks");
  createStructs();
  delay(500);

  textCard("Loading", "portfolio");
  buildOwnerList();
  //loadPortfolio();
  delay(500);

  // Initial updates
  for (int i = 0; i < numStocks; i++) {
    updateStockCurrent(stocks[i], true);
    delay(500);
  }

  textCard("Creating backgrnd", "processes...");
  delay(500);

  // Start background task on Core 1
  xTaskCreatePinnedToCore(
    stockUpdateTask,       // Task function
    "StockUpdater",        // Name
    8192,                  // Stack size
    NULL,                  // Parameter
    1,                     // Priority
    NULL,                  // Task handle
    1                      // Core ID (0 or 1)
  );
}

void loop() {
  timeCard();
  delay(5000);
  for(int i=0; i<numStocks; i++){
    stockDisplay(stocks[i].name.c_str(), stocks[i].currentValue, stocks[i].oneDay);
    delay(5000);
    
  }
  for(int i=0; i<ownerCount; i++){ //profit cards
    for(int j=0; j<numStocks; j++){
      if(userOwnsStock(owners[i], stocks[j].name.c_str())){
        ProfitResult p = getUserStockProfit(owners[i], stocks[j].name.c_str(), stocks, numStocks);
        profitsCard(owners[i], stocks[j].name.c_str(), p.oneDay, p.allTime);
        delay(10000); //10 sec
      }
    }
  }
  
}
