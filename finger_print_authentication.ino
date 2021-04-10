#include <SoftwareSerial.h>

// default module password and address. each 4 bytes
#define M_PASSWORD 0x00000000
#define M_ADDRESS 0xFFFFFFFF

// packet IDs. (High byte transferred first)
#define HEADER 0xEF01
#define PID_COMMAND 0x01
#define PID_DATA 0x02
#define PID_ACKNOWLEDGE 0x07
#define PID_PACKET_END 0x08
#define PID_DATA_END 0x08

// Module command codes
#define CMD_VERIFY_PASSWORD 0x13
#define CMD_COLLECT_FINGER_IMAGE 0x01

// length of buffer used to read serial data
#define FPS_DEFAULT_SERIAL_BUFFER_LENGTH 300

// default timeout to wait for response from scanner
#define DEFAULT_TIMEOUT 2000

SoftwareSerial sensorSerial(2, 3); // rx, tx -> yellow, white
Stream *commSerial = &sensorSerial;

// function to send packet to finger scanner
bool sendPacket(uint8_t pid, uint8_t cmd, uint8_t* data, uint16_t data_length){
  
  // seperate header and address into bytes
  uint8_t header_bytes[2];
  uint8_t address_bytes[4];

  header_bytes[0] = HEADER & 0xFF; // low byte
  header_bytes[1] = (HEADER >> 8) & 0xFF; // high byte
  
  address_bytes[0] = M_ADDRESS & 0xFF; // low byte
  address_bytes[1] = (M_ADDRESS >> 8) & 0xFF;
  address_bytes[2] = (M_ADDRESS >> 16) & 0xFF;
  address_bytes[3] = (M_ADDRESS >> 24) & 0xFF; // high byte
  
  if(data == NULL){
    data_length = 0;
    Serial.println("No data to send\n");

//    return false;
  }
  
  uint16_t packet_length = data_length + 3; // 1 byte command, 2 bytes checksum
  uint16_t check_sum = 0;
  uint8_t packet_length_in_bytes[2];
  uint8_t check_sum_in_bytes[2];
  
  packet_length_in_bytes[0] = packet_length & 0xFF; // get low byte
  packet_length_in_bytes[1] = (packet_length >> 8) & 0xFF; // get high byte

  check_sum = pid + packet_length_in_bytes[0] + packet_length_in_bytes[1] + cmd;

  for (int i = 0; i < data_length; i++){
    check_sum += data[i];
  }

  check_sum_in_bytes[0] = check_sum & 0xFF; // get low byte
  check_sum_in_bytes[1] = (check_sum >> 8) & 0xFF; // get high byte

  Serial.println("writing");
  // write to serial (tx). high bytes will be transferred first
  // header
  commSerial->write(header_bytes[1]);
  commSerial->write(header_bytes[0]);
  
  Serial.print(header_bytes[1], HEX);
  Serial.println(header_bytes[0], HEX);
  // address
  commSerial->write(address_bytes[3]);
  commSerial->write(address_bytes[2]);
  commSerial->write(address_bytes[1]);
  commSerial->write(address_bytes[0]);
  
  Serial.print(address_bytes[3], HEX);
  Serial.print(address_bytes[2], HEX);
  Serial.print(address_bytes[1], HEX);
  Serial.println(address_bytes[0], HEX);
  // packet id
  commSerial->write(pid);
  
  Serial.println(pid, HEX);
  // packet length
  commSerial->write(packet_length_in_bytes[1]);
  commSerial->write(packet_length_in_bytes[0]);
  
  Serial.print(packet_length_in_bytes[1], HEX);
  Serial.println(packet_length_in_bytes[0], HEX);
  // command
  commSerial->write(cmd);
  
  Serial.println(cmd, HEX);
  // data
  for(int i = (data_length - 1); i >= 0; i--){
    commSerial->write(data[i]);
    Serial.print(data[i], HEX);
  }
  Serial.println();
  // checksum
  commSerial->write(check_sum_in_bytes[1]);
  commSerial->write(check_sum_in_bytes[0]);
  
  Serial.print(check_sum_in_bytes[1], HEX);
  Serial.println(check_sum_in_bytes[0], HEX);
  
  Serial.println("written");

  return true;
}


// function to receive packet data from finger scanner
bool receivePacket(uint32_t timeout = DEFAULT_TIMEOUT){
  uint8_t* data_buffer = new uint8_t[64]; // data buffer must be 64 bytes
  uint8_t serial_buffer[FPS_DEFAULT_SERIAL_BUFFER_LENGTH] = {0}; // will store high byte at start of array
  uint16_t serial_buffer_length = 0;
  uint8_t byte_buffer = 0;

  // wait for response for set timout
  while(timeout > 0){
    if(commSerial->available()){
      byte_buffer = commSerial->read();
      Serial.println(byte_buffer, HEX);

      serial_buffer[serial_buffer_length] = byte_buffer;
      serial_buffer_length++;
    }

    timeout--;
    delay(1);
  }

  if(serial_buffer_length == 0){ // received nothing
    Serial.println("received nothing\n");
    return false;
  }

  if(serial_buffer_length < 10){ // bad packet
    Serial.print("bad packet nigga\n");
    return false;
  }

  uint16_t token = 0;   // to iterate over switch cases
  uint8_t packet_type;  // received packet type to use in checking checksum
  uint8_t rx_packet_length[2]; // received packet length
  uint16_t rx_packet_length_2b; // single data type for holding received packet length
  uint16_t data_length; // length of data extracted from packet length
  uint8_t confirmation_code;
  uint8_t checksum_bytes[2]; // checksum in bytes
  uint16_t checksum; // whole checksum representation

  //the following loop checks each segment of the data packet for errors
  while(1){
    switch(token){
      // case 0 & 1 check for start code
      case 0:
        // high byte
        if(serial_buffer[token] == ((HEADER >> 8) & 0xFF)){
          break;
        }
        
        return false;
        
      case 1:
        // low byte
        if(serial_buffer[token] == (HEADER & 0xFF)){
          break;
        }
        
        return false;
        
      // cases 2 to 5 check for device address. high to low byte
      case 2:
        if(serial_buffer[token] == ((M_ADDRESS >> 24) & 0xFF) ){
          break;
        }
        
        return false;

      case 3:
        if(serial_buffer[token] == ((M_ADDRESS >> 16) & 0xFF) ){
          break;
        }
        
        return false;

      case 4:
        if(serial_buffer[token] == ((M_ADDRESS >> 8) & 0xFF) ) {
          break;
        }
        
        return false;

      case 5:
        if(serial_buffer[token] == (M_ADDRESS & 0xFF) ) {
          break;
        }
        
        return false;

      // check for valid packet type {refactor to exclude command id}
      case 6:
        if((serial_buffer[token] == PID_COMMAND) || (serial_buffer[token] == PID_DATA) || (serial_buffer[token] == PID_ACKNOWLEDGE) || (serial_buffer[token] == PID_DATA_END)) {
          packet_type = serial_buffer[token];
          break;
        }
        
        return false;

      // read packet data length
      case 7:
        if((serial_buffer[token] > 0) || (serial_buffer[token + 1] > 0)){
          rx_packet_length[0] = serial_buffer[token + 1]; // low byte
          rx_packet_length[1] = serial_buffer[token]; // high byte
          rx_packet_length_2b = uint16_t(rx_packet_length[1] << 8) + rx_packet_length[0]; // the full length
          data_length = rx_packet_length_2b - 3; // 2 checksum , 1 conformation code
          
          token ++;

          break;
        }
        
        return false;

      // case 8 won't be hit as after case 7, token value is 9
      // read conformation code
      case 9:
        confirmation_code = serial_buffer[token];
        Serial.print("conformation code: ");
        Serial.println(confirmation_code, HEX);
        break;

      // read data
      case 10:
        for(int i = 0; i < data_length; i++){
          data_buffer[(data_length - 1) - i] = serial_buffer[token + i]; // low bytes will be written at end of array. Inherently, high bytes appear first
        }
        break;

      // read checksum
      case 11:
        // if there ain't no data
        if(data_length == 0){
          checksum_bytes[0] = serial_buffer[token]; // low byte
          checksum_bytes[1] = serial_buffer[token - 1]; // high byte
          checksum = uint16_t(checksum_bytes[1] << 8) + checksum_bytes[0];

          uint16_t tmp = 0;

          tmp = packet_type + rx_packet_length[0] + rx_packet_length[1] + confirmation_code;

          // if calculated checksum is equal to received checksum. is good to go
          if(tmp == checksum){
          Serial.println("pass");
            return true;
          }
//          Serial.println("fail");
          return false;

          break;
        }

        // if data is present
        else if((serial_buffer[token + (data_length -1)] > 0) || ((serial_buffer[token + 1 + (data_length -1)] > 0))){
          checksum_bytes[0] = serial_buffer[token + 1 + (data_length -1 )]; // low byte
          checksum_bytes[0] = serial_buffer[token + (data_length -1 )]; // high byte
          checksum = uint16_t (checksum_bytes[1] << 8) + checksum_bytes[0];

          uint16_t tmp = 0;

          tmp = packet_type + rx_packet_length[0] + rx_packet_length[1] + confirmation_code;

          for(int i=0; i < data_length; i++) {
            tmp += data_buffer[i]; //calculate data checksum
          }

          if(tmp == checksum) {
//            Serial.println("pass");
            return true;
          }
//          Serial.println("fail");
          return false;

          break;
        }

        // if checksum is 0
        else {
//          Serial.println("fail");
          return false;
        }

      default:
      Serial.println("default");
        break;
    }

    token ++;
  }
}

bool verifyPassword(uint32_t password = M_PASSWORD){
  // store password seperately in 4 bytes. isn't a necessity
  uint8_t password_bytes[4] = {0};
  password_bytes[0] = password & 0xFF;
  password_bytes[1] = (password >> 8);
  password_bytes[2] = (password >> 16);
  password_bytes[3] = (password >> 24);


  bool tx_response = sendPacket(PID_COMMAND, CMD_VERIFY_PASSWORD, password_bytes, 4);
  bool rx_response = receivePacket();
  
  if(rx_response){
    return true;
  }
  return false;
}

bool enrollFinger(){
  bool tx_response = sendPacket(PID_COMMAND, CMD_COLLECT_FINGER_IMAGE, NULL, 0);
   if(tx_response) Serial.println("Yes");
   else Serial.println("No");

  bool rx_response = receivePacket();
    if(rx_response) Serial.println("Yes");
    else Serial.println("No");
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  sensorSerial.begin(57600);

  enrollFinger();
//  bool response = verifyPassword();
//
//  if (response) Serial.println("Password Verified");
//  else Serial.println("Password Unverified");
}

void loop() {
  // put your main code here, to run repeatedly:
}
