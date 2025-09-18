
# TinyFarm
<img src="https://github.com/will060623/TinyFarm/blob/main/images/TinyFarm%20images.png?raw=true"/>

## what is TinyFarm

<img src="https://github.com/will060623/TinyFarm/blob/main/images/TinyFarm.png?raw=true"/>

 **TinyFarm** is real-time remote controllable Smartfarm using Arduino and **TinyIoT**.

 **TinyFarm** monitors sensor values in real time and sends them to **TinyIoT**.

 The user can control the actuator of the TinyFarm remotely.
<br> 


**About** 
[TinyIoT](https://github.com/seslabSJU/tinyIoT.git)

```
https://github.com/seslabSJU/tinyIoT.git
```


## File Structure of TinyFarm_Arduino

📂 TinyFarm_arduino
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;TinyFarm_arduino.ino          main code for arduino
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;DHT.cpp
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;DHT.h
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;LiquidCrystal_l2C.cpp
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;LiquidCrystal_l2C.h
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;RX9QR.cpp
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;RX9QR.h

### The **TinyFarm** code consists of the main code to upload to Arduino and the rest of the header file.

##  Installation & Configuration

Set an Ip address and wifi to prepare on environment for **TinyFarm**.

1. Download our code to your local

    ```
    https://github.com/will060623/TinyFarm.git
    ```
2. Configure your environment

    Modify the Ip address and wifi setting at the to of the code.

	```c
    //Server IP
    String server = "127.0.0.1";
    int port = 3000;
	```

    ```c
    //Wifi
    char ssid[] = "wifi name";
    char pass[] = "password"; 
    ```
3. Upload code
    
    Connect your pc to Arduino through USB port

    <img src="https://github.com/will060623/TinyFarm/blob/main/images/TinyFarm%20code%20Upload.png?raw=true"/>

    Open the code through the Arduino application and click the arrow to upload it.


## Development

### How to add sensor to TinyFarm
