/**
 This sketch is used for biometric verification using finger scanner r30x series.
 Written by Batman for Meem Enterprise.
 This sketch uses a library: BMA-R30X
 Git of the library:
 https://github.com/ParanoidBat/BMA-R30X
*/

#include <BMA_R30X.h>
#include "RTClib.h"

RTC_DS1307 rtc;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//#define GREEN_LED D5
//#define RED_LED D6

BMA bma;
char choice = '0';

void enroll(){
  if(bma.enrollFinger()) {
    bma.displayOLED("Registered Successfully");
//    digitalWrite(GREEN_LED, HIGH);
//    delay(1000);
//    digitalWrite(GREEN_LED, LOW);
  }
  else {
    bma.displayOLED("Try Again");
//    digitalWrite(RED_LED, HIGH);
//    delay(1000);
//    digitalWrite(RED_LED, LOW);
  }
}

void verifyFinger(){
  if(bma.fingerSearch()) {
    bma.displayOLED("Found Match");
//          Serial.print("ID: ");
//          Serial.println(bma.rx_data[bma.rx_data_length-2] + bma.rx_data[bma.rx_data_length-1], HEX);
  }
  else {
    bma.displayOLED("Couldn't Find A Match");
  }
  
  if(bma.rx_data != NULL) {
    delete [] bma.rx_data;
    bma.rx_data = NULL;
    bma.rx_data_length = 0;
  }
}

void setup() {
//  pinMode(GREEN_LED, OUTPUT);
//  pinMode(RED_LED, OUTPUT);
  
  Serial.begin(57600);
    
  if(bma.verifyPassword()) bma.displayOLED("Password Verified");
  else bma.displayOLED("Password Unverified");
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
    // When time needs to be set on a new device, or after a power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
//    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

DateTime now = rtc.now();

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
}

void loop() {
}
