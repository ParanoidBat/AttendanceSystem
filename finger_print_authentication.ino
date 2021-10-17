/**
 This sketch is used for biometric verification using finger scanner r30x series.
 Written by Batman for Meem Enterprise.
 This sketch uses a library: BMA-R30X
 Git of the library:
 https://github.com/ParanoidBat/BMA-R30X
*/

/**
 * OLED:
 * SCL -> D1
 * SDA -> D2
 */

#include <BMA_R30X.h>
#include <ESP8266WiFi.h>

#define CMD_ENROLL 0x64
#define CMD_VERIFY 0x65
#define CMD_READ_TEMPLATE_FROM_LIB 0x66

#define ACKNOWLEDGE 0x00
#define FAILURE 0x7d
#define TCP_BUFFER_LENGTH 2

String ssid = "NodeMcu";
String password = "11223344";

WiFiServer server(80);
WiFiClient client;

BMA bma;
char choice = '0';

void enroll(){
  if(bma.enrollFinger()) {
    bma.displayOLED("Registered Successfully");
    client.print(ACKNOWLEDGE);
  }
  else {
    bma.displayOLED("Try Again");
    client.print(FAILURE);
  }
}

void verifyFinger(){
  if(bma.fingerSearch()) {
    bma.displayOLED("Found Match");
//          Serial.print("ID: ");
//          Serial.println(bma.rx_data[bma.rx_data_length-2] + bma.rx_data[bma.rx_data_length-1], HEX);
    client.print(ACKNOWLEDGE);
  }
  else {
    bma.displayOLED("Couldn't Find A Match");
    client.print(FAILURE);
  }
  
  if(bma.rx_data != NULL) {
    delete [] bma.rx_data;
    bma.rx_data = NULL;
    bma.rx_data_length = 0;
  }
}

void readTcpBuffer(){
  byte tcp_buffer[TCP_BUFFER_LENGTH] = {0};
  byte buffer_index = 0;
  
  while(buffer_index < TCP_BUFFER_LENGTH){
    tcp_buffer[buffer_index] = client.read();
    buffer_index++;
  }

  if(buffer_index > 0){
    switch(tcp_buffer[1]){
      case CMD_ENROLL:
        enroll();
        break;

      case CMD_VERIFY:
        verifyFinger();
        break;

      case CMD_READ_TEMPLATE_FROM_LIB:
        bma.readTemplateFromLib();
        break;

      default:
        bma.displayOLED("Invalid Choice");
    }
  }
}

void setup() {
  Serial.begin(57600);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  server.begin();
  bool response = bma.verifyPassword();
  
  if(response) bma.displayOLED("Password Verified");
  else bma.displayOLED("Password Unverified");
}

void loop() {
  if(!client.connected()){
    client = server.available();
  }
  else {
    if(client.available() > 0){
      readTcpBuffer();
    }
    else{
      if(Serial.available() > 0){
//        client.print(Serial.readString());
      }
    }
  }
}
