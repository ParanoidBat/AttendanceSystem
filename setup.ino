#include <EEPROM.h>
#include "RTClib.h"

RTC_DS1307 rtc;

void setup() {
  Serial.begin(57600);

  EEPROM.begin(512);
  uint16_t location = 1;
  uint8_t count = 0;
  EEPROM.put(24, location);
  EEPROM.put(26, count);

  EEPROM.commit();
  EEPROM.end();

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  DateTime now = rtc.now();
  
  Serial.println(now.twelveHour());
  Serial.println(now.minute());
  Serial.println(now.second());

  delay(5000);
}
