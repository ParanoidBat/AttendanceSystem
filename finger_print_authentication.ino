/**
 This sketch is used for biometric verification using finger scanner r30x series.
 Written by Batman for Meem Enterprise.
 This sketch uses a library: BMA-R30X
 Git of the library:
 https://github.com/ParanoidBat/BMA-R30X
*/

#include <BMA_R30X.h>
#include "RTClib.h"
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266HTTPClient.h>

//#define GREEN_LED D5
//#define RED_LED D6

char* ssid = "ZONG4G-3D3A";
char* password = "02212165";
char* userUri = "https://bma-api-v1.herokuapp.com/user/";
char* attendanceUri = "https://bma-api-v1.herokuapp.com/attendance/";

uint16_t lumber = 2;
char str[29];

WiFiClientSecure httpsClient;
HTTPClient http;

RTC_DS1307 rtc;
DateTime now;

BMA bma;
char choice = '0';

int enroll(){
  if(bma.enrollFinger()) {
    httpsClient.connect(userUri, 443);
    http.begin(httpsClient, userUri);
    http.addHeader("Content-Type", "application/json");
    
    sprintf(str, "{\"name\":\"dummy\",\"authID\":%d}", lumber);
    bma.displayOLED("Registered Successfully");
  }
  else {
    bma.displayOLED("Try Again");
  }

  return http.POST(str);
}

int verifyFinger(){
  if(bma.fingerSearch()) {
    now = rtc.now();
    httpsClient.connect(attendanceUri, 443);
    http.begin(httpsClient, attendanceUri);
    http.addHeader("Content-Type", "application/json");
    
    sprintf(str, "{\"date\":\"%d/%d/%d\",\"timeIn\":\"%d:%d:%d\",\"authID\":2}", now.year(), now.month(), now.day(), now.twelveHour(), now.minute(), now.second());
    
//    bma.displayOLED("Found Match");
//    Serial.print("Found ID: ");
//    Serial.println(bma.rx_data[bma.rx_data_length-2] + bma.rx_data[bma.rx_data_length-1], HEX);
  }
  else {
//    bma.displayOLED("Couldn't Find A Match");
    Serial.println("couldnt find");
  }
  
  if(bma.rx_data != NULL) {
    delete [] bma.rx_data;
    bma.rx_data = NULL;
    bma.rx_data_length = 0;
  }

  return http.POST(str);
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

  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED){
    delay(500);
  }
  Serial.print("Wifi connected with ip: ");
  Serial.println(WiFi.localIP());

  httpsClient.setInsecure();
}

int responseCode;

void loop() {
    if(Serial.available()){
    choice = Serial.read();

    if(WiFi.status() == WL_CONNECTED){
      Serial.println("Sending request");

      switch(choice){
        case '1':
          responseCode = enroll();
          break;

        case '2':
          responseCode = verifyFinger();
          break;
          
        default:
          break;
      }

      if(responseCode < 0){
        Serial.printf("[HTTPS] failed, error: %s\n", http.errorToString(responseCode).c_str());
      }
      else{
        Serial.print("HTTPS Response code: ");
        Serial.println(responseCode);
        String payload = http.getString();
        Serial.println(payload);
        if(payload[2] == 'd'){
          Serial.println("Successfull.");
        }
        else{
          Serial.println("Some Error Occured. Try Again.");
        }
      }

      http.end();
    }
    else{
      Serial.println("Wifi disconnected.");
    }
  }
}
