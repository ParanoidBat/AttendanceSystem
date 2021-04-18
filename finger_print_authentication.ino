/*
 This sketch is used for biometric attendance using finger scanner r30x series.
 Written by Batman for Meem Enterprise.
 This sketch uses a library: BMA-R30X
 Git of the library:
 https://github.com/ParanoidBat/BMA-R30X
*/

#include <BMA_R30X.h>

SoftwareSerial sensorSerial(2, 3); // rx, tx -> yellow, white

BMA bma(&sensorSerial);
char choice = '0';

void setup() {
  Serial.begin(9600);
  
  bool response = bma.verifyPassword();
  
  if(response) Serial.println("Password Verified");
  else Serial.println("Password Unverified");
}

void loop() {
  Serial.println("1: Register \n2: Verify");
  
  while (Serial.available() <= 0) {}
  choice = Serial.read();

  switch(choice){
    case '1':
      if(bma.enrollFinger()) Serial.println("Registered");
      else Serial.println("Try Again");
      break;

     case '2':
      if(bma.fingerSearch()) {
        Serial.println("Found Match");
        Serial.print("ID: ");
        Serial.println(bma.rx_data[bma.rx_data_length-2] + bma.rx_data[bma.rx_data_length-1], HEX);
      }
      else Serial.println("Couldn't Find A Match");
      
      if(bma.rx_data != NULL) {
        delete [] bma.rx_data;
        bma.rx_data = NULL;
        bma.rx_data_length = 0;
       }
      break;
      
    default:
      Serial.println("Invalid Choice");
  }
}
