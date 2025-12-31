#ifndef TIME_OF_DAY_H
#define TIME_OF_DAY_H

#include <time.h>

// ===== New York Time Zone (EST / EDT with DST) =====
#define TZ_NEW_YORK "EST5EDT,M3.2.0/2,M11.1.0/2"

// ==================================================
// Returns: "Good morning", "Good afternoon", "Good evening"
// ==================================================
inline const char* getGreeting() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Hello";
  }

  int hour = timeinfo.tm_hour;

  if (hour >= 4 && hour < 14) {
    return "Good morning";
  } 
  else if (hour >= 14 && hour < 20) {
    return "Good afternoon";
  } 
  else {
    return "Good evening";
  }
}

bool isAwake() { //returns if the screen should be awake (6AM to 10PM)
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return true;
  }

  int hour = timeinfo.tm_hour;
  hour +=21;
  hour %= 24; //now its PST

  return (hour >= 6) && (hour <= 22);
}

// ==================================================
// Returns: "Market open" or "Market closed"
// NYSE hours: Mon–Fri, 9:30–16:00 ET
// ==================================================
inline const char* getMarketStatus() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return "Market closed";
  }

  int weekday = timeinfo.tm_wday; // 0 = Sunday
  int hour     = timeinfo.tm_hour;
  int minute   = timeinfo.tm_min;

  // Closed on weekends
  if (weekday == 0 || weekday == 6) {
    return "Market closed";
  }

  // Before open
  if (hour < 9 || (hour == 9 && minute < 30)) {
    return "Market closed";
  }

  // After close
  if (hour > 16 || (hour == 16 && minute > 0)) {
    return "Market closed";
  }

  return "Market open";
}

// ==================================================
// Returns military time string in EST/EDT
// Format: "HH:MM"
// ==================================================
inline const char* getESTMilitaryTime() {
  static char buffer[6]; // "HH:MM\0"
  struct tm timeinfo;

  if (!getLocalTime(&timeinfo)) {
    return "--:--";
  }

  snprintf(buffer, sizeof(buffer), "%02d:%02d",
           timeinfo.tm_hour, timeinfo.tm_min);

  return buffer;
}

#endif // TIME_OF_DAY_H
