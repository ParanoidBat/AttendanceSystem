/*
 This sketch is used for biometric verification using finger scanner r30x series.
 Written by Batman for Meem Enterprise.
 This sketch uses a library: BMA-R30X
 Git of the library:
 https://github.com/ParanoidBat/BMA-R30X
*/

#include <BMA_R30X.h>

BMA bma;
char choice = '0';

void setup() {
  Serial.begin(57600);
  
  bool response = bma.verifyPassword();
  
  if(response) Serial.println("Password Verified");
  else Serial.println("Password Unverified");
}

void loop() {
  Serial.println("1: Register \n2: Verify \n3: Read Template");
  
  while (Serial.available() <= 0) {}
  choice = Serial.read();

  switch(choice){
    case '1':
      if(bma.enrollFinger()) bma.displayOLED("Registered Successfully") ;
      else bma.displayOLED("Try Again");
      break;

     case '2':
      if(bma.fingerSearch()) {
        bma.displayOLED("Found Match");
        Serial.print("ID: ");
        Serial.println(bma.rx_data[bma.rx_data_length-2] + bma.rx_data[bma.rx_data_length-1], HEX);
      }
      else bma.displayOLED("Couldn't Find A Match");
      
      if(bma.rx_data != NULL) {
        delete [] bma.rx_data;
        bma.rx_data = NULL;
        bma.rx_data_length = 0;
       }
      break;

    case '3':
      bma.readTemplateFromLib();
      break;
    
    default:
      Serial.println("Invalid Choice");
  }
}
