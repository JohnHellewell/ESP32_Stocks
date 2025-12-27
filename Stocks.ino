#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <WiFiClientSecure.h>
#include <U8g2lib.h>
#include <Wire.h>

#include "secrets.h"
#include "stock.h"

// -------------------- Stock Symbols --------------------
String stockSymbols[] = {
  "QQQM",
  "AAPL",
  "AMZN",
  "MSFT",
  "GOOGL",
  "TSLA"
};

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, 22, 21);
#define X_OFFSET 0

const int numStocks = sizeof(stockSymbols) / sizeof(stockSymbols[0]);
Stock stocks[numStocks];

// -------------------- Display --------------------

const char* floatToString(float value, int width = 6, int precision = 2) {
  static char buffer[16];  // static so it persists after function ends
  dtostrf(value, width, precision, buffer);
  return buffer;
}

void display(const char *name, float currentPrice, float oneDayPrice) {
  u8g2.clearBuffer(); // Clear buffer
  u8g2.setFont(u8g2_font_7x13_tf);
  u8g2.drawStr(X_OFFSET, 10, "Hello, Hayden!");
  u8g2.drawStr(X_OFFSET, 30, name);
//  String temp = "$" + floatToString(currentPrice);
  u8g2.drawStr(80, 30, floatToString(currentPrice));
  float oneDayDifference = (oneDayPrice - currentPrice);
  u8g2.drawStr(75, 55, floatToString(oneDayDifference));
  u8g2.drawStr(X_OFFSET, 55, "24H");
  u8g2.drawHLine(0, 37, 140);
  u8g2.sendBuffer(); // Send buffer to display
}

// -------------------- WiFi --------------------
void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  // NTP for timestamps (optional)
  Serial.println("Starting NTP sync...");
  configTime(0, 0, "pool.ntp.org");

  time_t now = 0;
  unsigned long startAttempt = millis();
  while (now < 100000 && millis() - startAttempt < 10000) {
    now = time(nullptr);
    Serial.print("Waiting for NTP... epoch=");
    Serial.println(now);
    delay(500);
  }

  if (now < 100000) Serial.println("⚠️ NTP sync FAILED");
  else Serial.println("✅ NTP sync successful");
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

// -------------------- Stock Updates --------------------
// Finnhub: current price + 24h prev close
void updateStockCurrent(Stock &stock) {
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

// -------------------- Background Task --------------------
const unsigned long UPDATE_INTERVAL_CURRENT = 60000; // 1 min

void stockUpdateTask(void *parameter) {
  for (;;) { // infinite loop for FreeRTOS task
    for (int i = 0; i < numStocks; i++) {
      updateStockCurrent(stocks[i]);
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
  createStructs();

  // Initial updates
  for (int i = 0; i < numStocks; i++) {
    updateStockCurrent(stocks[i]);
    delay(500);
  }

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
  for(int i=0; i<numStocks; i++){
    display(stocks[i].name.c_str(), stocks[i].currentValue, stocks[i].oneDay);
    delay(5000);
  }
  
}
