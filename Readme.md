# TinyFarm

<img src="https://github.com/will060623/TinyFarm/blob/main/images/TinyFarm%20images.png?raw=true" />

## 1. What is TinyFarm?

<img src="https://github.com/will060623/TinyFarm/blob/main/images/Tiny.drawio.png?raw=true" />

**TinyFarm** is a real-time, remotely controllable smart farm built with **Arduino** and **TinyIoT**.

- TinyFarm reads sensor values (temperature, humidity, CO2, Soilmoisture) in real time.
- It sends those values to **TinyIoT**.
- The user can **remotely control actuators** (LED, fan, pump, door) from the TinyIoT side.

**About TinyIoT**  
[TinyIoT GitHub](https://github.com/seslabSJU/tinyIoT.git)
```
https://github.com/seslabSJU/tinyIoT.git
```

## 2. Repository Structure

📂 TinyFarm_arduino
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;TinyFarm_arduino.ino          main code for arduino
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;TinyFarm_ESP32.ino            code for ESP32
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;NotifyServer.py               code for NotifyServer
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;DHT.cpp
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;DHT.h
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;LiquidCrystal_l2C.cpp
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;LiquidCrystal_l2C.h
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;RX9QR.cpp
   <br>&nbsp;&nbsp;&nbsp;&nbsp;|- &nbsp;RX9QR.h

   ## 3. Prerequisites
   - Arduino IDE installed
   - ESP32 board package installed in Arduino IDE (for TinyFarm_ESP32.ino)
   - A TinyIoT server running and reachable (public IP or same LAN)
   - Raspberry Pi (Ubuntu) if you want to run the notify server
   - Python 3 + virtual environment (for Flask)


   ## 4. Setup for Arduino / ESP32
   ### 4.1 Download the code
   Clone or download this repository to your PC:
   ```
   git clone https://github.com/will060623/TinyFarm1.git
   ```
   Open the <mark>.ino</mark> files in **Arduino IDE**.  
   ### 4.2 Configuration network settings
   Both Arduino(TinyFarm_arduino.ino) and ESP32(TinyFarm_ESP32.ino) have the same style of config at the top.
   
   Server (TinyIoT) address:

```c
//Server IP
String server = "127.0.0.1";
int port = 3000;
```

- Change 127.0.0.1 to your TinyIoT server IP (ex. 192.168.0.10 or public IP).
- Keep the port the same as TinyIoT.

    Wi-Fi settings:

```c
//Server IP
String server = "127.0.0.1";
int port = 3000;
```

- Change to your actual Wi-Fi SSID / password.

- Make sure Arduino/ESP32 and TinyIoT server are on the same network (or TinyIoT is publicly accessible).

### 4.3 Select board & upload
1. Connect Arduino / ESP32 via USB.

2. In Arduino IDE:

<img src="https://github.com/will060623/TinyFarm/blob/main/images/%EC%8A%A4%ED%81%AC%EB%A6%B0%EC%83%B7%202025-10-30%20175502.png?raw=true"/>

   - Tools → Board → select your board (Arduino UNO/MEGA / ESP32 DevKit)

   - Tools → Port → select the right serial port

3. Click Upload 

<img src="https://github.com/will060623/TinyFarm/blob/main/images/TinyFarm%20code%20Upload.png?raw=true" />

## 5. Notify Server Setup
You need this if you want TinyIoT → TinyFarm direction

### 5.1 Prepare Raspberry Pi (Ubuntu)
Install Ubuntu/Raspberry Pi OS:
```
https://www.raspberrypi.com/software/operating-systems/
```
Clone the project on the Pi:
```
git clone https://github.com/will060623/TinyFarm.git
cd TinyFarm
```

### 5.2 Create virtual environment and install Flask
```
python3 -m venv venv
source venv/bin/activate
pip install Flask
```
### 5.3 Run the notify server
```
python3 NotifyServer.py
```
### 5.4 Register in TinyIoT
In your TinyIoT server, register the notify server as the subscription notification server:

**Register the notify server as the subscription notification server in TinyIoT.**

## 6. Notes
   - If TinyIoT is on **public IP** and Raspberry Pi is on **private IP**, you must set up port forwarding on your router so TinyIoT can  reach NotifyServer.py.

   - The Arduino/ESP32 side periodically POST sensor values to TinyIoT.

   - The notify server receives commands sent from TinyIoT and forwards them to the Arduino over serial.

## 7. Quick Flow Summary
1. Arduino/ESP32 → TinyIoT : sensor data upload

2. TinyIoT → NotifyServer (Flask) : subscription notification

3. NotifyServer → Arduino (serial) : actual actuator command
