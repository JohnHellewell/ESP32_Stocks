#include "secrets.h"

WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

String url = "https://finnhub.io/api/v1/quote?symbol=AAPL&token=" FINNHUB_API_KEY;


void setup() {
  Serial.begin(115200);

  

}

void loop() {
  // put your main code here, to run repeatedly:

}
