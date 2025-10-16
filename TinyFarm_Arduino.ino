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
int status_sensor = 0;

RX9QR RX9(cal_A, cal_B, Base_line, meti, mein, 700, 1000, 2000, 4000);

String cmd1 = "OFF";
String cmd2 = "OFF";
String cmd3 = "0";
String cmd4 = "OFF";

float temperature, humidity;
int angle = 0;
int RBG_R = 35; 
int RBG_G = 35; 
int RBG_B = 36;
int waterValue = 0;
int lightoutput = 0;
char sData[64] = { 0x00, };
char nData[32] = { 0x00, };
int resource_sum = 0;

unsigned Lux; // add code - Lee  

#include <WiFiEsp.h> 
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

String server = "203.250.148.89";
int port = 3000;

WiFiEspClient client;
HttpClient http(client, server , port);

char ssid[] = "sejong-guest";   
char pass[] = "0234083114";  
//0234083114

int status = WL_IDLE_STATUS;  // the Wifi radio's status
WiFiEspServer server_f(400);
//========================================================
//ESP32 시리얼 설정
#include <SoftwareSerial.h>
SoftwareSerial ESP32(50,51);


// 정수를 문자열로 바꾸어 반환하는 함수
/* Convert unsigned int to String */
String unsignedToString(unsigned int value) {
    String result;
    do {
        result = char('0' + (value % 10)) + result;
        value /= 10;
    } while (value > 0);
    return result;
}

// 처음 아두이노 실행 시 리소스트리를 생성하는 함수 (1차 리소스트리 반영 테스트용)
// 나머지는 리소스트리 스크립트로 수동 생성하기


DHT dht(DHTPIN, DHTTYPE);
Servo servo; 
LiquidCrystal_I2C lcd(0x27, 16, 2);

String getCommandFromTinyIoT(String path) {
  String payload = "";

  if (!client.connect(server.c_str(), port)) {
    Serial.println("TinyIoT 연결 실패");
    return "";
  }

  // 요청 전송
  String req = "GET " + path + " HTTP/1.1\r\n";
  req += "Host: " + server + ":" + port + "\r\n";
  req += "X-M2M-RI: retrieve_cin\r\n";
  req += "X-M2M-Rvi: 2a\r\n";
  req += "X-M2M-Origin: CAdmin\r\n";
  req += "Accept: application/json\r\n";
  req += "Connection: close\r\n\r\n";
  client.print(req);

  // 1) 헤더를 바이트 단위로 읽으며 \r\n\r\n 탐지
  String headers; headers.reserve(512);
  int match = 0;             // CRLFCRLF 매칭 진행도
  unsigned long t0 = millis();
  const unsigned long HDR_TIMEOUT = 5000;

  while (millis() - t0 < HDR_TIMEOUT && match < 4) {
    if (client.available()) {
      char c = client.read();
      headers += c;
      t0 = millis();

      // CRLFCRLF 탐지
      if ((match == 0 && c == '\r') ||
          (match == 1 && c == '\n') ||
          (match == 2 && c == '\r') ||
          (match == 3 && c == '\n')) {
        match++;
      } else {
        match = (c == '\r') ? 1 : 0; // 부분 일치 리셋
      }
    }
  }

  if (match < 4) {
    Serial.println("헤더 읽기 실패(경계 미검출)");
    client.stop();
    return "";
  }

  // 2) Content-Length 추출
  int contentLength = -1;
  {
    int idx = headers.indexOf("Content-Length:");
    if (idx < 0) idx = headers.indexOf("Content-length:");
    if (idx >= 0) {
      int eol = headers.indexOf("\r\n", idx);
      String len = headers.substring(idx + 15, eol);
      len.trim();
      contentLength = len.toInt();
    }
  }

  // (선택) 상태코드 확인
  int status = -1;
  {
    int s = headers.indexOf("HTTP/1.1 ");
    if (s >= 0 && s + 12 <= (int)headers.length()) {
      status = headers.substring(s + 9, s + 12).toInt();
    }
  }

  // 3) 본문 N바이트 정확히 읽기 (N 미상일 땐 close까지)
  String body;
  if (contentLength > 0) body.reserve(contentLength + 8);
  t0 = millis();
  const unsigned long BODY_TIMEOUT = 5000;

  if (contentLength >= 0) {
    while ((int)body.length() < contentLength && millis() - t0 < BODY_TIMEOUT) {
      if (client.available()) {
        char c = client.read();
        body += c;
        t0 = millis();
      }
    }
  } else {
    // Content-Length 없을 때: 연결 종료까지
    while ((client.connected() || client.available()) && millis() - t0 < BODY_TIMEOUT) {
      if (client.available()) {
        char c = client.read();
        body += c;
        t0 = millis();
      }
    }
  }

  client.stop();

  // Content-Length 지정됐으면 초과분 제거
  if (contentLength >= 0 && (int)body.length() > contentLength) {
    body = body.substring(0, contentLength);
  }

  // 디버그 (필요 시만)
  // Serial.println("=== RAW PAYLOAD START ===");
  // Serial.println(body);
  // Serial.println("=== RAW PAYLOAD END ===");

  // 4) JSON 파싱
  StaticJsonDocument<512> doc;   // 필요시 768~1024로
  DeserializationError err = deserializeJson(doc, body);
  if (err) {
    Serial.print("JSON 파싱 실패: ");
    Serial.println(err.f_str());
    return "";
  }

  if (doc["m2m:cin"]["con"]) {
    return String((const char*)doc["m2m:cin"]["con"]);
  }

  Serial.println("con 필드 없음");
  return "";
}

// 액정 화면 출력 함수
// 스마트팜 농장의 LCD 화면에 현재 센서 값이 출력되게함
void printLCD(int col, int row , char *str) {
    for(int i=0 ; i < strlen(str) ; i++){
      lcd.setCursor(col+i , row);
      lcd.print(str[i]);
    }
  }


// 와이파이 환경 연결함수
void printWifiStatus(){
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


// 초기 1회 setup 함수
// 시리얼 포트로 연결 시 최초 1회만 실행됨됨
void setup() {

  pinMode(LIGHTPIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);
  pinMode(RBG_R,OUTPUT);
  pinMode(RBG_G,OUTPUT);
  pinMode(RBG_B,OUTPUT);
  digitalWrite(FAN_PIN, LOW);
  Serial.begin(9600);
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
  printLCD(0, 0, "Smartfarm Project");
  printLCD(0, 1, "NETWORKING...");  

#if USE_NETWORK
  // initialize serial for ESP module
  Serial2.begin(9600);
  // initialize ESP module
  WiFi.init(&Serial2);

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    #if DEBUG
    Serial.println("WiFi shield not present");
  #endif
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    #if DEBUG
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
  #endif
    // Connect to WPA/WPA2 network
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
  int tem;
  int hum;
  int co;

    
  float h = dht.readHumidity();        // 보통 readHumidity()
  hum = (int)h;

  float t = dht.readTemperature();     // ℃
  tem = (int)t;
 
  int SoilValue = analogRead(1);
  SoilValue = (10230 - SoilValue * 10) / 100;

  float EMF  = analogRead(EMF_pin);
  EMF = EMF / (ADCResol - 1) * ADCvolt / 6 * 1000;
  float THER = analogRead(THER_pin);
  THER = 1 / (C1 + C2 * log((Resist_0 * THER)/(ADCResol - THER)) + C3 * pow(log((Resist_0 * THER)/(ADCResol - THER)), 3)) - 273.15;
  int co2_ppm = RX9.cal_co2(EMF, THER) / 10;
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

  Serial.println("보내는 중");

  String cmd1 = getCommandFromTinyIoT("/TinyIoT/TinyFarm/Actuators/Door/la");
  String cmd2 = getCommandFromTinyIoT("/TinyIoT/TinyFarm/Actuators/Fan/la"); 
  String cmd3 = getCommandFromTinyIoT("/TinyIoT/TinyFarm/Actuators/LED/la");
  String cmd4 = getCommandFromTinyIoT("/TinyIoT/TinyFarm/Actuators/Water/la");

    if (cmd1.length() > 0) {
      const char* nData = cmd1.c_str();
      if (strcmp(nData, "ON") == 0) {
          angle = 10;
          servo.attach(SERVOPIN);
          servo.write(angle);
          delay(500);
          servo.detach();
          Serial.println("[창문 제어] 열림 (10도)");
      }
      else if (strcmp(nData, "OFF") == 0) {
          angle = 80;
          servo.attach(SERVOPIN);
          servo.write(angle);
          delay(500);
          servo.detach();
          Serial.println("[창문 제어] 닫힘 (80도)");
      }

    }

    Serial.println(cmd2);
    if (cmd2.length() > 0){
      digitalWrite(FAN_PIN, HIGH);
      const char* nData = cmd2.c_str();

      if(memcmp(nData, "ON", 2) == 0) {

        digitalWrite(FAN_PIN, HIGH);
        Serial.print("[팬 제어] FAN=");
        Serial.println((nData[0] == 'O' && nData[1] == 'N') ? 1 : 0);
      }
      else if(memcmp(nData, "OFF", 3) == 0) {

        digitalWrite(FAN_PIN, LOW);
        Serial.print("[팬 제어] FAN=");
        Serial.println((nData[0] == 'O' && nData[1] == 'N') ? 1 : 0);
      }
    }


    if (cmd3.length() > 0){ 

        const char* nData = cmd3.c_str();
        int light = atoi(nData);
        
        lightoutput = light;
        analogWrite(LIGHTPIN, 25 * light);
        analogWrite(RBG_R, 25 * light);
        analogWrite(RBG_G, 25 * light);
        analogWrite(RBG_B, 25 * light);
        Serial.print("[조명 제어] 밝기=");
        Serial.println(light);
      
    }

      if (cmd4.length() > 0){ 

       const char* nData = cmd4.c_str();

       if(memcmp(nData, "ON", 2) == 0) {
         digitalWrite(WATER_PUMP_PIN, (nData[0] == 'O' && nData[1] == 'N') ? HIGH : LOW);
         Serial.print("[펌프 제어] WATER=");
         Serial.println((nData[0] == 'O' && nData[1] == 'N') ? 1 : 0);
       }

       if(memcmp(nData, "OFF", 3) == 0) {
         digitalWrite(WATER_PUMP_PIN, (nData[0] == 'O' && nData[1] == 'N') ? HIGH : LOW);
         Serial.print("[펌프 제어] WATER=");
         Serial.println((nData[0] == 'O' && nData[1] == 'N') ? 1 : 0);
       }

      }

      //센서값 LCD 출력

      lcd.clear();
        memset(sData, 0x00, 64);
        sprintf(sData, "temp %02dC humi %02d%%", tem,
        hum);
        printLCD(0, 0, sData);
        memset(sData, 0x00, 64);
        sprintf(sData, "co2 %-04d soil %-04d", co, waterValue);
        printLCD(0, 1, sData);

      sprintf(sData, "{ \"temp\":%02d,\"Humidity\":%02d,\"co2\":%-04d,\"water\":%-04d }", tem, hum, co, waterValue);
    
      Serial1.println(sData);
  
  
    delay(1); // 또는 yield();

    }
