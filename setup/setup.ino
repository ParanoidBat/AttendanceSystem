#include <EEPROM.h>

void setup() {
  Serial.begin(57600);
  EEPROM.begin(512);
  uint16_t location = 1;
  uint8_t count = 0;
  EEPROM.put(24, location);
  EEPROM.put(26, count);

  EEPROM.commit();
  EEPROM.end();

  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
