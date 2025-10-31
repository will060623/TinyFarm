#include <Servo.h>
#include "DHT.h"

#include <Wire.h>
#include "LiquidCrystal_I2C.h"

#define DHTPIN 12
#define DHTTYPE DHT11
#define SERVOPIN 9
#define LIGHTPIN 4
#define FAN_PIN 32
#define WATER_PUMP_PIN 31
#define USE_NETWORK 1
#define USE_BLUETOOTH 1
#define DEBUG 1
#define RESOURCE_CREATED_SUM 8

extern void RX9QR_loop(); 
extern unsigned int co2_ppm = 0;

#include "RX9QR.h"
#define EMF_pin A2   // RX-9 E with A0 of arduino
#define THER_pin A3  // RX-9 T with A1 of arduino
#define ADCvolt 5
#define ADCResol 1024
#define Base_line 432
#define meti 60  
#define mein 120 //Automotive: 120, Home or indoor: 1440

//CO2 calibrated number
float cal_A = 383.3; // you can take the data from RX-9 bottom side QR data #### of first 4 digits. you type the data to cal_A as ###.#
float cal_B = 62.76; // following 4 digits after cal_A is cal_B, type the data to cal_B as ##.##

// Thermister constant
// RX-9 have thermistor inside of sensor package. this thermistor check the temperature of sensor to compensate the data
// don't edit the number
#define C1 0.00230088
#define C2 0.000224
#define C3 0.00000002113323296
float Resist_0 = 15;
float EMF = 0;
float THER = 0;
String cmd1 = "OFF";
String cmd2 = "OFF";
String cmd3 = "0";
String cmd4 = "OFF";


RX9QR RX9(cal_A, cal_B, Base_line, meti, mein, 700, 1000, 2000, 4000);

int angle = 0;
int RBG_R = 35; 
int RBG_G = 35; 
int RBG_B = 36;
int waterValue = 0;
char sData[64] = { 0x00, };
char nData[32] = { 0x00, };
int resource_sum = 0;


#include <WiFiEsp.h> 
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

String server = "127.0.0.1";     //your TinyIoT address
int port = 3000;                      //port

WiFiEspClient client;
HttpClient http(client, server , port);

char ssid[] = "wifiname";        //wifi name
char pass[] = "password";          //password

int status = WL_IDLE_STATUS;  
WiFiEspServer server_f(400);


#include <SoftwareSerial.h>      //ESP32 Serial Config
SoftwareSerial ESP32(50,51);     //Esp board(RX,TX)


DHT dht(DHTPIN, DHTTYPE);
Servo servo; 
LiquidCrystal_I2C lcd(0x27, 16, 2);


void printLCD(int col, int row , char *str) {       //print LCD
    for(int i=0 ; i < strlen(str) ; i++){
      lcd.setCursor(col+i , row);
      lcd.print(str[i]);
    }
  }


void printWifiStatus(){                 //connect wifi
#if 1 // #if DEBUG   
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
#endif

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  delay(10);
#if 1 // #if DEBUG
  Serial.print("IP Address: ");
  Serial.println(ip);
#endif
  char ipno2[26] ;
  sprintf(ipno2, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  printLCD(0, 1, ipno2);

  // print the received signal strength
  long rssi = WiFi.RSSI();
}

void setup() {

  pinMode(LIGHTPIN, OUTPUT);       //pin config
  pinMode(FAN_PIN, OUTPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);
  pinMode(RBG_R,OUTPUT);
  pinMode(RBG_G,OUTPUT);
  pinMode(RBG_B,OUTPUT);
  digitalWrite(FAN_PIN, LOW);
  Serial.begin(115200);
  Serial.setTimeout(50);    
  Serial1.begin(9600);
  Serial2.begin(9600); 
  ESP32.begin(9600);
  dht.begin();
  int flag = 0;

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  lcd.init();
  lcd.backlight();
  printLCD(0, 0, (char *)"Smartfarm Project");
  printLCD(0, 1, (char *)"NETWORKING...");  

#if USE_NETWORK                             //wifi setting
  // initialize serial for ESP module
  Serial2.begin(9600);
  // initialize ESP module
  WiFi.init(&Serial2);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    #if DEBUG
    Serial.println("WiFi shield not present");
    #endif
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    #if DEBUG
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    #endif
    status = WiFi.begin(ssid, pass);
  }
  #if DEBUG
  Serial.println("You're connected to the network");
  #endif
  printWifiStatus(); // display IP address on LCD
  delay(2000);
  
  server_f.begin(); 
#endif
  
  #if DEBUG
  Serial.println("START");
  #endif

  if(resource_sum == RESOURCE_CREATED_SUM) {
    Serial.println("SmartFarm Succesfully Registered");  
  }
  else{
    Serial.println("Failed to register SmartFarm");
  }
}

void loop() {
  while (Serial.available() > 0) {             //read Actuator status value from Raspberry pi
    String line = Serial.readStringUntil('\n');   
    line.trim();                                 
    if (line.length() == 0) continue;

    Serial.print("[RX] raw='");
    Serial.print(line);
    Serial.println("'");

    int sep = line.indexOf(':');
    if (sep <= 0 || sep == line.length() - 1) {
      Serial.println("[RX] skip: no ':' or empty value");
      continue;
    }

    String key = line.substring(0, sep); key.trim();
    String val = line.substring(sep + 1); val.trim();

    Serial.print("[RX] key='"); Serial.print(key);
    Serial.print("' val='");    Serial.print(val);
    Serial.println("'");

    if (key.equalsIgnoreCase("LEDsub1")) {
      cmd3 =  val;   // brightness "0~10"
    } else if (key.equalsIgnoreCase("Doorsub1")) {
      cmd1 = val;    // "ON"/"OFF"
    } else if (key.equalsIgnoreCase("Fansub1")) {
      cmd2 = val;    // "ON"/"OFF"
    } else if (key.equalsIgnoreCase("Watersub1")) {
      cmd4 = val;    // "ON"/"OFF"
    } else {
      Serial.println("[RX] unknown key (ignored)");
    }
  }

  int tem;
  int hum;
  int co;

  float h = dht.readHumidity();        //Humidity
  hum = (int)h;

  float t = dht.readTemperature();     //Temperature
  tem = (int)t;
 
  int SoilValue = analogRead(1);         //SoilWater
  SoilValue = (10230 - SoilValue * 10) / 100;

  float EMF  = analogRead(EMF_pin); 
  EMF = EMF / (ADCResol - 1) * ADCvolt / 6 * 1000;
  float THER = analogRead(THER_pin);
  THER = 1 / (C1 + C2 * log((Resist_0 * THER)/(ADCResol - THER)) + C3 * pow(log((Resist_0 * THER)/(ADCResol - THER)), 3)) - 273.15;
  int co2_ppm = RX9.cal_co2(EMF, THER) / 10;                    //CO2
  String C = String(co2_ppm);

  Serial.print("Hum :");
  Serial.println(hum);
  Serial.print("Temp :");
  Serial.println(tem);  
  Serial.print("Soil :");  
  Serial.println(SoilValue);
  Serial.print("CO2 :");  
  Serial.println(co2_ppm);

  Serial1.print("Hum :");
  Serial1.println(hum);

  Serial1.print("Temp :");
  Serial1.println(tem);

  Serial1.print("CO2 :");
  Serial1.println(co2_ppm);

  Serial1.print("Soil :");
  Serial1.println(SoilValue);

  Serial.println("Sending");

  if (cmd1.length() > 0) {                    //Door
    const char* nData = cmd1.c_str();
    if (strcmp(nData, "ON") == 0) {
      angle = 10;
      servo.attach(SERVOPIN);
      servo.write(angle);
      delay(500);
      servo.detach();
      Serial.println("[Door] Open");
    } else if (strcmp(nData, "OFF") == 0) {
      angle = 80;
      servo.attach(SERVOPIN);
      servo.write(angle);
      delay(500);
      servo.detach();
      Serial.println("[Door] Close");
    } else {
      Serial.println("[Door] error");
    }
    cmd1 = "";  
  }

  if (cmd2.length() > 0) {               //Fan
    const char* nData = cmd2.c_str();
    if (memcmp(nData, "ON", 2) == 0) {
      digitalWrite(FAN_PIN, HIGH);
      Serial.println("[Fan] ON");
    } else if (memcmp(nData, "OFF", 3) == 0) {
      digitalWrite(FAN_PIN, LOW);
      Serial.println("[Fan] OFF");
    } else {
      Serial.println("[Fan] error");
    }
    cmd2 = "";  
  }

  if (cmd3.length() > 0) {             //LED
    int light = cmd3.toInt();          
    light = constrain(light, 0, 10);
    analogWrite(LIGHTPIN, 25 * light);
    analogWrite(RBG_R,   25 * light);
    analogWrite(RBG_G,   25 * light);
    analogWrite(RBG_B,   25 * light);
    Serial.print("[LED] brightness");
    Serial.println(light);
    cmd3 = "";  
  }
 
  if (cmd4.length() > 0) {              //Pump
    const char* nData = cmd4.c_str();
    if (memcmp(nData, "ON", 2) == 0) {
      digitalWrite(WATER_PUMP_PIN, HIGH);
      Serial.println("[pump] ON");
    } else if (memcmp(nData, "OFF", 3) == 0) {
      digitalWrite(WATER_PUMP_PIN, LOW);
      Serial.println("[pump] OFF");
    } else {
      Serial.println("[pump] error");
    }
    cmd4 = "";  
  }

  lcd.clear();
  memset(sData, 0x00, 64);
  sprintf(sData, "temp %02dC humi %02d%%", tem, hum);
  printLCD(0, 0, sData);
  memset(sData, 0x00, 64);
  sprintf(sData, "co2 %-04d soil %-04d", co, waterValue);
  printLCD(0, 1, sData);

  sprintf(sData, "{ \"temp\":%02d,\"Humidity\":%02d,\"co2\":%-04d,\"water\":%-04d }", tem, hum, co, waterValue);
  Serial1.println(sData);

  delay(1); 
}
