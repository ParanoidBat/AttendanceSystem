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

// For communication with Android client
#define ACKNOWLEDGE 0x00
#define FAILURE 0x01
#define TCP_HEADER '2'
#define CMD_NETWORK '4'
#define CMD_PASSWORD '5'
#define CMD_FINISH '9'

//#define BTN_ENROLL D3
//#define BTN_SETUP D4
//#define BTN_CHECKIN D0
//#define BTN_CHECKOUT D5
#define LED_RED D6
#define LED_GREEN D7
#define LED_BLUE D8

char* ssid = "ZONG4G-3D3A";
char* password = "02212165";
char* userUri = "https://bma-api-v1.herokuapp.com/user/";
char* attendanceUri = "https://bma-api-v1.herokuapp.com/attendance/";
char body[109];

WiFiClientSecure httpsClient;
HTTPClient http;

RTC_DS1307 rtc;
DateTime now;

BMA bma;
char choice = '0';
int responseCode;
uint32_t start_counter = millis();
uint8_t attendances = 0;
Attendance attendance;

void turnOnLED(String led){
  if (led == "red"){
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(LED_RED, HIGH);
  }
  else if (led == "green"){
    digitalWrite(LED_BLUE, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
  }
  else if (led == "blue"){
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_BLUE, HIGH);
  }
}

bool writeEEPROMAtSetup(String key, String value){
  if(key == "network") {
    EEPROM.put(NETWORK_LENGTH, (uint8_t)value.length());
    
    for (uint8_t i = 2, j = 0; j < value.length(); i++, j++) EEPROM.put(i, value[j]);
  }
  else if(key == "password") {
    uint8_t ssid_len;
    EEPROM.get(NETWORK_LENGTH, ssid_len);
    EEPROM.put(PASSWORD_LENGTH, (uint8_t)value.length());
    
    for (uint8_t i = 2 + ssid_len, j = 0; j < value.length(); i++, j++ ) EEPROM.put(i, value[j]);
  }
  else if(key == "id") {
    uint8_t ssid_len, pass_len;
    EEPROM.get(NETWORK_LENGTH, ssid_len);
    EEPROM.get(PASSWORD_LENGTH, pass_len);

    for (uint8_t i = 2 + ssid_len + pass_len, j = 0; j < value.length(); i++, j++) EEPROM.put(i, value[j]);

    bma.finger_location = 2 + ssid_len + pass_len + 24;
    bma.attendance_count = 2 + ssid_len + pass_len + 24 + 2;
    bma.attendance_store = 2 + ssid_len + pass_len + 24 + 2 + 1;
    
    EEPROM.put(bma.finger_location, (uint16_t)0);
    EEPROM.put(bma.attendance_count, (uint8_t)0);
  }
  else return false;
  
  EEPROM.commit();
  return true;
}

void initialSetup(){
  turnOnLED("blue");
  
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

      if(response != "" && response[0] == TCP_HEADER){
        String data = response.substring(2);
        if(response[1] == CMD_FINISH){
          if(writeEEPROMAtSetup("id", data)) {
            client.print(ACKNOWLEDGE);
            client.flush();
          }
          else client.print(FAILURE);
          
          EEPROM.end();
          
          WiFi.softAPdisconnect(true);
          WiFi.mode(WIFI_STA);
          WiFi.begin(ssid, password);
          delay(2000);

          if (WiFi.status() == WL_CONNECTED){
            Serial.print("Wifi connected with ip: ");
            Serial.println(WiFi.localIP());
          }
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
    
          default:
            client.print(FAILURE);
        }
      }
      else client.print(0x08);
    }
  }
}

void enroll(){
  turnOnLED("blue");
  
  uint16_t authID = bma.enrollFinger();
  if(authID > 0) {
    if(WiFi.status() == WL_CONNECTED){
      httpsClient.connect(userUri, 443);
      http.begin(httpsClient, userUri);
      http.addHeader("Content-Type", "application/json");

      sprintf(body, "{\"name\":\"dummy\",\"authID\":%d,\"organizationID\":\"%s\"}", authID, bma.organizationID.c_str());
      responseCode = http.POST(body);
  
      if(responseCode < 0) {
        bma.displayOLED("Try Again.");
        turnOnLED("red");
        Serial.printf("[HTTPS] failed, error: %s\n", http.errorToString(responseCode).c_str());
      }
      else{
        Serial.print("HTTPS Response code: ");
        Serial.println(responseCode);
        String payload = http.getString();
        Serial.println(payload);
        if(payload[2] == 'd'){
          turnOnLED("green");
          sprintf(body, "Finger ID: %d", authID);
          bma.displayOLED(body);
        }
        else {
          bma.displayOLED("Try Again.");
          turnOnLED("red");
        }
      }
      http.end();
    }
    else {
      turnOnLED("red");
      bma.displayOLED("No Network..");
    }
  }
  else {
    turnOnLED("red");
    bma.displayOLED("Try Again.");
  }
}

void verifyFinger(){
  turnOnLED("blue");
  
  if(bma.fingerSearch()) {
    uint16_t authID = bma.rx_data[bma.rx_data_length-2] + bma.rx_data[bma.rx_data_length-1];
    Serial.print("\nauthID: "); Serial.println(authID);
    now = rtc.now();

    if(WiFi.status() == WL_CONNECTED){
      Serial.println(F("sending"));
      sprintf(body, "{\"date\":\"%d-%d-%d\",\"timeIn\":\"%d:%d:%d\",\"authID\":%d,\"organizationID\":\"%s\"}",
        now.year(), now.month(), now.day(),
        now.twelveHour(), now.minute(), now.second(),
        authID, bma.organizationID.c_str());

      httpsClient.connect(attendanceUri, 443);
      http.begin(httpsClient, attendanceUri);
      http.addHeader("Content-Type", "application/json");
      responseCode = http.POST(body);

      if(responseCode < 0) {
        turnOnLED("red");
        Serial.printf("[HTTPS] failed, error: %s\n", http.errorToString(responseCode).c_str());
      }
      else{
        Serial.print("HTTPS Response code: ");
        Serial.println(responseCode);
        String payload = http.getString();
        Serial.println(payload);
        if(payload[2] == 'd'){
          bma.displayOLED("Successfull");
          turnOnLED("green");
        }
        else {
          turnOnLED("red");
          bma.displayOLED("Try Again");
        }
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
      
      EEPROM.get(bma.attendance_count, count);
      if(count > -1){
        int location = bma.attendance_store + count * 9;
        // TODO: check if there is capacity before putting in
        EEPROM.put(location, att);
        EEPROM.put(bma.attendance_count, count+1);

        EEPROM.commit();
        EEPROM.end();

        bma.displayOLED("Successfull.");
        turnOnLED("green");
      }
      else {
        turnOnLED("red");
        bma.displayOLED("Try Again.");
      }
    }
  }
  else {
    bma.displayOLED("No Match.");
    turnOnLED("red");
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
  delay(2000);
  
  if(WiFi.status() == WL_CONNECTED){
    Serial.print("Wifi connected with ip: ");
    Serial.println(WiFi.localIP());
  }
  
  httpsClient.setInsecure();

//  pinMode(BTN_ENROLL, INPUT);
//  pinMode(BTN_SETUP, INPUT);
//  pinMode(BTN_CHECKIN, INPUT);
//  pinMode(BTN_CHECKOUT, INPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(LED_GREEN, LOW);
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

      case '3':
        initialSetup();
        
      default:
        break;
    }
 }
//  if (digitalRead(BTN_ENROLL) == HIGH) {
//    Serial.println("enroll");
//    enroll();
//  }
//  else if (digitalRead(BTN_SETUP) == HIGH) {
//    Serial.println("setup");
//    initialSetup();
//  }
//  else if (digitalRead(BTN_CHECKIN) == HIGH) {
//    Serial.println("verify");
//    verifyFinger();
//  }

 if(millis() - start_counter > 1800000){ // 30 mins
  start_counter = millis();
  EEPROM.begin(512);
  EEPROM.get(bma.attendance_count, attendances);

  if(attendances > 0 && WiFi.status() == WL_CONNECTED){
    bma.displayOLED("Uploading Data..");
    httpsClient.connect(attendanceUri, 443);
    http.begin(httpsClient, attendanceUri);
    http.addHeader("Content-Type", "application/json");
    
    while(attendances > 0){
      EEPROM.get((bma.attendance_store + (attendances -1) * 9), attendance); // implemented as stack
      
      sprintf(body, "{\"date\":\"%d/%d/%d\",\"timeIn\":\"%d:%d:%d\",\"authID\":%d,\"organizationID\":\"%s\"}",
        attendance.current_year, attendance.current_month, attendance.current_date,
        attendance.current_hour, attendance.current_minute, attendance.current_second,
        attendance.authID, bma.organizationID.c_str());
        
      responseCode = http.POST(body);
  
      if(responseCode < 0) {
        Serial.printf("[HTTPS] failed, error: %s\n", http.errorToString(responseCode).c_str());
        break;
      }
      if(WiFi.status() != WL_CONNECTED) {
        Serial.println("Wifi disconnected");
        bma.displayOLED("No Network");
        break;
      }
      
      attendances --;
      EEPROM.put(bma.attendance_count, attendances);
      EEPROM.commit();
    }
  }
  http.end();
  EEPROM.end();
 }
}
