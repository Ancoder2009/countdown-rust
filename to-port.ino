#include <DS3231.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <Wire.h>
#include <SD.h>

DS3231 rtc(SDA, SCL);
LiquidCrystal_I2C lcd(0x27, 20, 4);

struct Count {
  int days;
  int hours;
  int mins;
  int secs;
  bool over;
  bool overflowOver;
};

struct Target {
  unsigned long timestamp;
  String message;
};

String center(String text) {
  String printing = "";
  for (int i = 0; i < floor((20 - text.length()) / 2); i++) {
    printing = printing + " ";
  }
  printing = printing + text;
  for (int i = 0; i < ceil((20 - text.length()) / 2); i++) {
    printing = printing + " ";
  }
  return printing;
}

void writeCountLine(int line, String text) {
  lcd.setCursor(0, line);
  lcd.print(center(text));
}

String generateTextFromTimeLeft(Count count) {
  char s[32];
  snprintf(s, sizeof(s), "%02i:%02i:%02i:%02i", count.days, count.hours, count.mins, count.secs);
  return String(s);
}

unsigned long lastTick = 0;
int lastHour;
bool night = false;
bool over = false;
bool update = true;
Target target;
String contents;
int DST = 0;

Count getCount(unsigned long target) {
  Count count;
  long n = target - rtc.getUnixTime(rtc.getTime()) + (DST * 3600);
  count.overflowOver = ((n + 86400) < -1);
  if (n < 0) {
    count.over = true;
    return count;
  }
  float days = floor(n / 86400);
  float hours = floor((n - (days * 86400)) / 3600);
  float mins = floor((n - (days * 86400) - (hours * 3600)) / 60);
  float secs = floor((n - (days * 86400) - (hours * 3600) - (mins * 60)));
  count.days = days;
  count.hours = hours;
  count.mins = mins;
  count.secs = secs;
  count.over = false;
  return count;
}

bool setupTarget(String candidate) {
  if (candidate.indexOf(";") > 0) {
    String message = candidate.substring(0, candidate.indexOf(";"));
    unsigned long timestamp = candidate.substring(candidate.indexOf(";") + 1).toInt();
    Count candidateCount = getCount(timestamp);
    if (candidateCount.overflowOver) {
      return false;
    }
    target = Target{ timestamp, message };
    return true;
  }
  return false;
}

void start(String data) {
  lcd.clear();
  night = false;
  over = false;
  char *token = strtok(data.c_str(), "\n");
  bool up = false;
  while (token != NULL) {
    if (setupTarget(String(token))) {
      up = true;
      break;
    }
    token = strtok(NULL, "\n");
  }
  if (!up) {
    update = false;
    writeCountLine(2, "No More Targets");
    return;
  }
  writeCountLine(1, center(target.message));
}

void setup() {
  setPinMode(2, INPUT_PULLUP);
  // Begin Modules
  // Serial
  Serial.begin(9600);
  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  writeCountLine(2, "Initializing...");
  // RTC
  rtc.begin();
  // SD
  if (!SD.begin(10)) {
    update = false;
    writeCountLine(2, "SD Card Init Error");
    return;
  }

  // Get Data
  if (!SD.exists("DATA")) {
    update = false;
    writeCountLine(2, "SD File Not Found");
    return;
  }
  File f = SD.open("DATA");
  contents = f.readString();
  f.close();
  start(contents);
  if (!SD.exists("DST")) {
    update = false;
    writeCountLine(2, "DST File Not Found");
    return;
  }
  f = SD.open("DST");
  contents = f.readString();
  f.close();
  if (contents == "true") {
    DST = 1;
  }
}

void loop() {
  unsigned long currentTick = millis();
  if ((unsigned long)(currentTick - lastTick) >= 1000) {
    if (update) {
      Count count = getCount(target.timestamp);
      if (!night && !over) {
        // 1698739200
        writeCountLine(2, generateTextFromTimeLeft(count));
        if (count.over) {
          writeCountLine(2, "Countdown Over");
          over = true;
        }
      }
      if (count.overflowOver) {
        start(contents);
      }
    }

    int hour = rtc.getTime().hour + DST;
    if (hour != lastHour) {
      if (hour >= 21) {
        lcd.noBacklight();
        writeCountLine(2, "Night Mode!");
        night = true;
      } else if (hour < 7) {
        lcd.noBacklight();
        writeCountLine(2, "Night Mode!");
        night = true;
      } else {
        lcd.backlight();
        night = false;
      }
    }
    lastHour = hour;

    lastTick = currentTick;
  }
  if (digitalRead(2) == HIGH) {
    lcd.clear();
    writeCountLine(2, "Reloading Data...");
    if (!SD.begin(10)) {
      update = false;
      writeCountLine(2, "SD Card Init Error");
      return;
    }
    if (!SD.exists("DATA")) {
      update = false;
      writeCountLine(2, "SD File Not Found");
      return;
    }
    File f = SD.open("DATA");
    contents = f.readString();
    f.close();
    start(contents);
    update = true;
  }
}
