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

#define ACKNOWLEDGE 0x00
#define FAILURE 0x01
#define HEADER '2'
#define CMD_ID '3'
#define CMD_NETWORK '4'
#define CMD_PASSWORD '5'
#define CMD_FINISH '9'

char* ssid = "ZONG4G-3D3A";
char* password = "02212165";
char* userUri = "https://bma-api-v1.herokuapp.com/user/";
char* attendanceUri = "https://bma-api-v1.herokuapp.com/attendance/";
char body[29];

WiFiClientSecure httpsClient;
HTTPClient http;

RTC_DS1307 rtc;
DateTime now;

BMA bma;
char choice = '0';
int responseCode;
uint32_t start_counter = millis();
uint8_t attendance_count = 0;
Attendance attendance;

bool writeEEPROMAtSetup(String key, String value){
  if(key == "netwrok") EEPROM.put(NETWORK, value);
  else if(key == "password") EEPROM.put(PASSWORD, value);
  else if(key == "id") EEPROM.put(ORGANIZATION, value);
  else return false;
  
  EEPROM.commit();
  return true;
}

void initialSetup(){
  String ap_ssid = "BMA-Fv1";
  String ap_password = "11223344";
  
  WiFiServer server(80);
  WiFiClient client;

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  server.begin();
  Serial.println("Listening on server.");

  EEPROM.begin(512);
  
  while(1){
    if(!client.connected()){
     client = server.available();
    }
    else if(client.available() > 0){
      String response = client.readStringUntil('/');
    
      if(response != "" && response[0] == HEADER){
        String data = response.substring(2);
       
        if(response[1] == CMD_FINISH){
          EEPROM.end();
          client.print(ACKNOWLEDGE);
          WiFi.softAPdisconnect(true);
          WiFi.mode(WIFI_STA);
          break;
        }
        
        switch(response[1]){
          case CMD_NETWORK:
            if(writeEEPROMAtSetup("network", data)) client.print(ACKNOWLEDGE);
            else client.print(FAILURE);
            break;

          case CMD_PASSWORD:
            if(writeEEPROMAtSetup("password", data)) client.print(ACKNOWLEDGE);
            else client.print(FAILURE);
            break;

          case CMD_ID:
            if(writeEEPROMAtSetup("id", data)) client.print(ACKNOWLEDGE);
            else client.print(FAILURE);
            break;
    
          default:
            client.print(FAILURE);
        }
      }
      else client.print(0x08);
    }
  }
}

void enroll(){
  uint16_t authID = bma.enrollFinger();
  if(authID > 0) {
    if(WiFi.status() == WL_CONNECTED){
      httpsClient.connect(userUri, 443);
      http.begin(httpsClient, userUri);
      http.addHeader("Content-Type", "application/json");
      
      sprintf(body, "{\"name\":\"dummy\",\"authID\":%d}", authID);
      responseCode = http.POST(body);
  
      if(responseCode < 0) Serial.printf("[HTTPS] failed, error: %s\n", http.errorToString(responseCode).c_str());
      else{
        Serial.print("HTTPS Response code: ");
        Serial.println(responseCode);
        String payload = http.getString();
        Serial.println(payload);
        if(payload[2] == 'd'){
          Serial.println("Successfull.");
          sprintf(body, "Finger ID: %d", authID);
          bma.displayOLED(body);
        }
        else Serial.println("Some Error Occured. Try Again.");
      }
      http.end();
    }
    else bma.displayOLED("No Network..");
  }
  else bma.displayOLED("Try Again.");
}

void verifyFinger(){
  if(bma.fingerSearch()) {
    uint16_t authID = bma.rx_data[bma.rx_data_length-2] + bma.rx_data[bma.rx_data_length-1];
    now = rtc.now();

    if(WiFi.status() == WL_CONNECTED){
      sprintf(body, "{\"date\":\"%d/%d/%d\",\"timeIn\":\"%d:%d:%d\",\"authID\":%d}",
        now.year(), now.month(), now.day(),
        now.twelveHour(), now.minute(), now.second(),
        authID);
        
      httpsClient.connect(attendanceUri, 443);
      http.begin(httpsClient, attendanceUri);
      http.addHeader("Content-Type", "application/json");
      responseCode = http.POST(body);
  
      if(responseCode < 0) Serial.printf("[HTTPS] failed, error: %s\n", http.errorToString(responseCode).c_str());
      else{
        Serial.print("HTTPS Response code: ");
        Serial.println(responseCode);
        String payload = http.getString();
        Serial.println(payload);
        if(payload[2] == 'd'){
          bma.displayOLED("Successfull");
        }
        else bma.displayOLED("Try Again.");
      }

      http.end();
    }
    else{
      Attendance att{
        authID,
        now.year(),
        now.month(),
        now.day(),
        now.twelveHour(),
        now.minute(),
        now.second()
      };

      EEPROM.begin(512);
      uint8_t count = -1;
      
      EEPROM.get(ATTENDANCE_COUNT, count);
      if(count > -1){
        uint16_t location = ATTENDANCE_STORE + count * 9;
        EEPROM.put(location, att);
        EEPROM.put(ATTENDANCE_COUNT, count+1);

        EEPROM.commit();
        EEPROM.end();

        bma.displayOLED("Successfull.");
      }
      else bma.displayOLED("Try Again.");
    }
  }
  else {
    bma.displayOLED("No Match.");
  }
  
  if(bma.rx_data != NULL) {
    delete [] bma.rx_data;
    bma.rx_data = NULL;
    bma.rx_data_length = 0;
  }
}

void setup() {  
  Serial.begin(57600);
    
  if(bma.verifyPassword()) bma.displayOLED("Password Verified");
  else bma.displayOLED("Password Unverified");
  
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
  }
  if (! rtc.isrunning()) {
    Serial.println("RTC is NOT running, let's set the time!");
  }

  WiFi.begin(ssid, password);
//  while(WiFi.status() != WL_CONNECTED){
//    delay(500);
//  }
  
  Serial.print("Wifi connected with ip: ");
  Serial.println(WiFi.localIP());

  httpsClient.setInsecure();
}

void loop() {
  if(Serial.available()){
    choice = Serial.read();

    switch(choice){
      case '1':
        enroll();
        break;

      case '2':
        verifyFinger();
        break;
        
      default:
        break;
    }
 }

 if(millis() - start_counter > 1800000){ // 30 mins
  start_counter = millis();
  EEPROM.begin(512);
  EEPROM.get(ATTENDANCE_COUNT, attendance_count);

  if(attendance_count > 0 && WiFi.status() == WL_CONNECTED){
    httpsClient.connect(attendanceUri, 443);
    http.begin(httpsClient, attendanceUri);
    http.addHeader("Content-Type", "application/json");
    
    while(attendance_count > 0){
      EEPROM.get((ATTENDANCE_STORE + (attendance_count -1) * 9), attendance); // implemented as stack
      
      sprintf(body, "{\"date\":\"%d/%d/%d\",\"timeIn\":\"%d:%d:%d\",\"authID\":%d}",
        attendance.current_year, attendance.current_month, attendance.current_date,
        attendance.current_hour, attendance.current_minute, attendance.current_second,
        attendance.authID);
        
      responseCode = http.POST(body);
  
      if(responseCode < 0) {
        Serial.printf("[HTTPS] failed, error: %s\n", http.errorToString(responseCode).c_str());
        break;
      }
      if(WiFi.status() != WL_CONNECTED) {
        Serial.println("Wifi disconnected");
        break;
      }
      
      attendance_count --;
      EEPROM.put(ATTENDANCE_COUNT, attendance_count);
      EEPROM.commit();
    }
  }
  http.end();
  EEPROM.end();
 }
}
