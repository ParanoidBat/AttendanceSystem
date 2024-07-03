Here you will find the main Arduino code sketches, plus a proteus project folder that houses the Attendance System's circuit schema.  
You will first upload the `setup.ino` sketch to your device, and then the `finger_print_authentication.ino`.  
## How this works

If you have a look at the circuit schema, you will notice it does not have a keypad. This device instead comminicates with a mobile phone (one of the selling points), for whom, an application is made.  
At first startup, the device will need to be setup; for which there is a button on the device (D4 : `BTN_SETUP`). Pressing the button will put the device in **soft AP** mode, the mobile phone is supposed to connect to it and provide it with your WiFi credentials.  
The device will then attempt to register itself on the database. On subsequent startups (though it is recommended that the device will stay active forever), the device will automatically connect with the WiFi.  

To enroll, there is also a button (D3 : `BTN_ENROLL`) on the device. After the fingerprint is registered, the device will display a number; which is the **fingerprint ID**, you will need this number to put in the mobile phone interface for registering the new user.  

The code should be understandable now that you know the flow of the system. The `RTClib` library; I believe, needs to be downloaded from Arduino's official library archive.  
The `BMA_R30X` library can be found [here](https://github.com/ParanoidBat/BMA-R30X)  
The mobile application can be found [here](https://github.com/ParanoidBat/BMA-Client-App)
