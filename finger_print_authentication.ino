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

#define BTN_ENROLL D3
#define BTN_SETUP D4
#define BTN_CHECKIN D0
#define BTN_CHECKOUT D5
#define LED_RED D6
#define LED_GREEN D7
#define LED_BLUE D8

char* ssid = "ZONG4G-3D3A";
char* password = "02212165";
char* userUri = "https://bma-api-v1.herokuapp.com/user/create_from_device";
char* checkInUri = "https://bma-api-v1.herokuapp.com/attendance/";
char* checkOutUri = "https://bma-api-v1.herokuapp.com/attendance/checkout";
char body[109];
byte state = 1; // button state

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

    // 5 bytes for organization id
    bma.finger_location = 2 + ssid_len + pass_len + 5;
    bma.attendance_count = 2 + ssid_len + pass_len + 5 + 2;
    bma.attendance_store = 2 + ssid_len + pass_len + 5 + 2 + 1;
    
    EEPROM.put(bma.finger_location, (uint16_t)0);
    EEPROM.put(bma.attendance_count, (uint8_t)0);
  }
  else return false;
  
  EEPROM.commit();
  return true;
}

String generateSSID(uint8_t *mac){
  String ssid = "";
  
  for(int i = 0; i < 6; i++){
    uint8_t r = mac[i];
    if(r > 122){
      uint8_t x = mac[i]%122;
      
      if(x > 122) r = 122 - x;
      else if(x < 48) r = 48 + x;
      else r = x;
    }
    else if(r < 48){
      if (r == 0) r = 48;
      else r = 48%r + 48;
    }
    ssid += (char)r;
  }

  return ssid;
}

String generatePassword(String ssid){
  String password = "00";

  for(int i = 4; i < ssid.length(); i++){
    password += ssid[i] + 5;
  }

  return password;
}

void initialSetup(){
  turnOnLED("blue");

  uint8_t mac[6];
  WiFi.macAddress(mac);

  String string_mac = String(mac[5]);
  for(int i = 4; i >= 0; i--){
    string_mac += ":";
    string_mac += mac[i];
  }
  
  String ap_ssid = "BMA-" + generateSSID(mac);
  String ap_password = generatePassword(ap_ssid);
  
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

            httpsClient.connect("https://bma-api-v1.herokuapp.com/user/org_product/", 443);
            http.begin(httpsClient, userUri);
            http.addHeader("Content-Type", "application/json");

            char org_product_body[84];
            sprintf(org_product_body, "{\"org_id\":\"%s\",\"product_name\":\"FP1\",\"product_mac\":\"%s\"}", bma.organizationID.c_str(), string_mac.c_str());
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
            }
            else {
              bma.displayOLED("Try Again.");
              turnOnLED("red");
            }
          }
          http.end();
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
      http.addHeader("x-access-token", "bma_token_03");

      sprintf(body, "{\"name\":\"dummy\",\"finger_id\":%d,\"organization_id\":\"%s\"}", authID, bma.organizationID.c_str());

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

void check_in(){
  turnOnLED("blue");
  
  if(bma.fingerSearch()) {
    uint16_t authID = bma.rx_data[bma.rx_data_length-2] + bma.rx_data[bma.rx_data_length-1];
    Serial.print("\nauthID: "); Serial.println(authID);
    now = rtc.now();

    if(WiFi.status() == WL_CONNECTED){
      Serial.println(F("sending"));
      sprintf(body, "{\"date\":\"%d-%d-%d\",\"check_in\":\"%d:%d:%d\",\"finger_id\":%d,\"organization_id\":\"%s\"}",
        now.year(), now.month(), now.day(),
        now.twelveHour(), now.minute(), now.second(),
        authID, bma.organizationID.c_str());

      httpsClient.connect(checkInUri, 443);
      http.begin(httpsClient, checkInUri);
      http.addHeader("Content-Type", "application/json");
      http.addHeader("x-access-token", "bma_token_03");
      
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

void check_out(){
  turnOnLED("blue");
  
  if(bma.fingerSearch()) {
    uint16_t authID = bma.rx_data[bma.rx_data_length-2] + bma.rx_data[bma.rx_data_length-1];
    Serial.print("\nauthID: "); Serial.println(authID);
    now = rtc.now();

    if(WiFi.status() == WL_CONNECTED){
      Serial.println(F("sending"));
      sprintf(body, "{\"date\":\"%d-%d-%d\",\"check_out\":\"%d:%d:%d\",\"finger_id\":%d,\"organization_id\":\"%s\"}",
        now.year(), now.month(), now.day(),
        now.twelveHour(), now.minute(), now.second(),
        authID, bma.organizationID.c_str());

      httpsClient.connect(checkOutUri, 443);
      http.begin(httpsClient, checkOutUri);
      http.addHeader("Content-Type", "application/json");
      http.addHeader("x-access-token", "bma_token_03");
      
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
          bma.displayOLED("Checked out");
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
      bma.displayOLED("No connection");
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

  pinMode(BTN_ENROLL, INPUT_PULLUP);
  pinMode(BTN_SETUP, INPUT_PULLUP);
  pinMode(BTN_CHECKIN, INPUT_PULLUP);
  pinMode(BTN_CHECKOUT, INPUT_PULLUP);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);

  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_BLUE, HIGH);
  digitalWrite(LED_GREEN, LOW);
}

void loop() {
  if (!digitalRead(BTN_ENROLL)) {
    if(state != 3){
      state = 3;
      enroll();
    }
  }
  else if (state == 3) state = 1;
  
  if (!digitalRead(BTN_SETUP)) {
    if (state != 4){
      state = 4;
      initialSetup();      
    }
  }
  else if (state == 4) state = 1;
  
  if (!digitalRead(BTN_CHECKIN)) {
    if (state != 0){
      state = 0;
      check_in();      
    }
  }
  else if (state == 0) state = 1;

  if (!digitalRead(BTN_CHECKOUT)) {
    if (state != 5){
      state = 5;
      check_out();
    }
  }
  else if (state == 5) state = 1;

 if(millis() - start_counter > 1800000){ // 30 mins
  start_counter = millis();
  EEPROM.begin(512);
  EEPROM.get(bma.attendance_count, attendances);

  if(attendances > 0 && WiFi.status() == WL_CONNECTED){
    bma.displayOLED("Uploading Data..");
    httpsClient.connect(checkInUri, 443);
    http.begin(httpsClient, checkInUri);
    http.addHeader("Content-Type", "application/json");
    
    while(attendances > 0){
      EEPROM.get((bma.attendance_store + (attendances -1) * 9), attendance); // implemented as stack
      
      sprintf(body, "{\"date\":\"%d/%d/%d\",\"check_in\":\"%d:%d:%d\",\"finger_id\":%d,\"organizationI_id\":\"%s\"}",
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
