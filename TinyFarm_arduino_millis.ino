//20250725 아두이노 업로드 할 코드
//이산화탄소코드 포함
#include <Servo.h>
#include "DHT.h"
#define DHTPIN 12
#define DHTTYPE DHT11
#define SERVOPIN 9
#define LIGHTPIN 4
#define FAN_PIN 32
#define WATER_PUMP_PIN 31
#include <Wire.h>
#include "LiquidCrystal_I2C.h"
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

// 원하는 전송 주기 — 예시값
const unsigned long HUM_POST_MS  = 10UL * 1000;   // 습도 10초
const unsigned long TEMP_POST_MS = 10UL * 1000;   // 온도 10초
const unsigned long SOIL_POST_MS = 30UL * 1000;   // 토양수분 30초
const unsigned long CO2_POST_MS  = 60UL * 1000;   // CO2 60초

// 마지막 전송 시각
unsigned long lastHumPost  = 0;
unsigned long lastTempPost = 0;
unsigned long lastSoilPost = 0;
unsigned long lastCO2Post  = 0;

// TinyIoT 원격명령 주기
const unsigned long CMD_POLL_MS = 5UL * 1000;
unsigned long lastCmdPoll = 0;

float temperature, humidity;
int angle = 0;
int RBG_R = 35; 
int RBG_G = 35; 
int RBG_B = 36;
int cdcValue = 0;
int waterValue = 0;
int lightoutput = 0;
int fanOutput = 0;
int waterPumpPin = 0;
int timeout = 11; 
char sData[64] = { 0x00, };
char rData[32] = { 0x00, };
char nData[32] = { 0x00, };
int rPos = 0;
int nPos = 0;
int right = 10;
int resource_sum = 0;

unsigned Lux; // add code - Lee  

#include <WiFiEsp.h> 
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

WiFiEspClient client;
HttpClient http(client, "172.19.8.237", 3000);

String server = "172.19.8.237";
int port = 3000;

char ssid[] = "sejong-guest";   // your network SSID (name) 와이파이 이름을 우선 세종대학교 공용 와이파이로 함
char pass[] = "0234083114";  // your network password 비밀번호 입력
//0234083114

int status = WL_IDLE_STATUS;  // the Wifi radio's status
WiFiEspServer server_f(400);
//========================================================

bool displayToggle = false;

// TinyIoT로 get요청하는 함수
// 파싱하고 헤더 포함해서 요청 보내기
int get(String path) { 

  // HTTP Request Header
  String request = "GET " + path + " HTTP/1.1\r\n";
  request += "Host: ";
  request += server;
  request += ":";
  request += port;
  request += "\r\n";
  request += "X-M2M-RI: retrieve\r\n";
  request += "X-M2M-Rvi: 2a\r\n";
  request += "X-M2M-Origin: CAdmin\r\n";
  request += "Accept: application/json\r\n";
  request += "Connection: close\r\n\r\n";

  // Print Request Log
  // Serial.println(F("---- HTTP REQUEST START ----"));
  // Serial.println(request);
  // Serial.println(F("---- HTTP REQUEST END ----"));

  // Send Request with headers
  if (client.connect(server.c_str(), port)) {
    client.print(request);
    Serial.println(F("\nGet Request sent"));
    delay(1000);
    
    int status = -1;
    // Get Response
    while (client.connected()) {
      while (client.available()) {
        String line = client.readStringUntil('\n');
        if(line.startsWith("HTTP/1.1")){
          status = line.substring(9,12).toInt();
        }
        delay(100);
      }
      client.stop();
    }
    Serial.println(status);
    if(status == 200 || status == 201){
      Serial.println(F("Get Response received"));
    }
    return status == 200 || status == 201 ? 1 : 0;
  } else {
    Serial.println(F("Connection to server failed"));
    return -1;
  }
}


//TinyIoT로 post요청하는 함수
// 파싱하고 헤더 포함해서 요청 보내기
int post(String path, String contentType, String name, String content){
  // HTTP Request Header
  String request = "POST " + path + " HTTP/1.1\r\n";
  request += "Host: ";
  request += server;
  request += ":";
  request += port;
  request += "\r\n";
  request += "X-M2M-RI: create\r\n";
  request += "X-M2M-Rvi: 2a\r\n";
  request += "X-M2M-Origin: SArduino\r\n";
  
  if(contentType.equals("AE")){
    request += "Content-Type: application/json;ty=2\r\n";
  } else if(contentType.equals("CNT")){
    request += "Content-Type: application/json;ty=3\r\n";
  } else if(contentType.equals("CIN")){
    request += "Content-Type: application/json;ty=4\r\n";
  } else{
    Serial.println(F("Unvalid Content Type!")); 
    return 0;
  }
  
  // Serialize Json Body
  String body = serializeJsonBody(contentType, name, content);

  request += "Content-Length: " + unsignedToString(body.length())  + "\r\n";
  request += "Accept: application/json\r\n";
  request += "Connection: close\r\n\r\n";
  

  // Print Request Log
  // Serial.println(F("---- HTTP REQUEST START ----"));
  // Serial.println(request + body);
  // Serial.println(F("---- HTTP REQUEST END ----"));

  // Send Request with headers and body
 if (client.connect(server.c_str(), port)) {
    client.print(request + body);
    Serial.println(F("\nPost Request sent"));
    delay(1000);
    
    int status = -1;
    // Get Response
     while (client.connected()) {
      while (client.available()) {
        String line = client.readStringUntil('\n');
        if(line.startsWith("HTTP/1.1")){
          status = line.substring(9,12).toInt();
        }
        delay(100);
      }
      client.stop();
    }
    Serial.println(status);
    if(status == 200 || status == 201){
      Serial.println(F("Post Response Received"));
    }
    return status == 200 || status == 201 ? 1 : 0;
  } else {
    Serial.println(F("Connection to server failed"));
    return -1;
  }
}

// TinyIoT에서 get요청하여 필요한 contentInstance를 파싱하는 함수
String getCommandFromTinyIoT(String path) {
  
  String payload = "";

  if (client.connect(server.c_str(), port)) {
    String request = "GET " + path + " HTTP/1.1\r\n";
    request += "Host: " + server + ":" + port + "\r\n";
    request += "X-M2M-RI: retrieve\r\n";
    request += "X-M2M-Rvi: 2a\r\n";
    request += "X-M2M-Origin: CAdmin\r\n";
    request += "Accept: application/json\r\n";
    request += "Connection: close\r\n\r\n";

    client.print(request);
    delay(500);

    bool inBody = false;
    while (client.connected()) {
      while (client.available()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") inBody = true;
        else if (inBody) payload += line + "\n";
      }
    }
    client.stop();
  } else {
    Serial.println("TinyIoT 연결 실패");
    return "";
  }

  // JSON 파싱
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, payload);
  if (!error && doc["m2m:cin"]["con"]) {
    return String((const char*)doc["m2m:cin"]["con"]);
  }

  return "";
}




// json 데이터 body를 생성하는 함수
String serializeJsonBody(String contentType, String name, String content){
  String body;
  JsonDocument doc;

  // Constructs JsonDocument 
  if(contentType.equals("AE")){
    JsonObject m2m_ae = doc["m2m:ae"].to<JsonObject>();
    m2m_ae["rn"] = name;
    m2m_ae["api"] = "NTinyFarm";
    JsonArray lbl = m2m_ae["lbl"].to<JsonArray>();
    lbl.add("ae_"+name);
    m2m_ae["srv"][0] = "3";
    m2m_ae["rr"] = true;
  } else if(contentType.equals("CNT")){
    JsonObject m2m_cnt = doc["m2m:cnt"].to<JsonObject>();
    m2m_cnt["rn"] = name;
    JsonArray lbl = m2m_cnt["lbl"].to<JsonArray>();
    lbl.add("cnt_"+name);
    m2m_cnt["mbs"] = 16384;
  } else if(contentType.equals("CIN")){
    JsonObject m2m_cin = doc["m2m:cin"].to<JsonObject>();
    m2m_cin["con"] = content;
  }
  
  doc.shrinkToFit();
  serializeJson(doc, body);

  return body;
}



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
void setDevice(){
  Serial.println(F("Setup Started!"));
  //startMelody();
  int fflag = 0;

  /* Check Arduino AE exists in TinyIoT if not create Arduino AE, CNT */
  int status = get("/TinyIoT/TinyFarm");
  if(status == 1) {
    Serial.println(F("Successfully retrieved!"));
    resource_sum = RESOURCE_CREATED_SUM;
  } 
  else if(status == 0){
    Serial.println(F("Not Found... Creating AE"));

    int state = post("/TinyIoT","AE","TinyFarm","");
    if(state == 1){
      Serial.println(F("Successfully created AE_TinyFarm"));
      resource_sum ++;

      int state_CNT1 = post("/TinyIoT/TinyFarm", "CNT", "Sensors", "");
      if(state_CNT1 == 1){
        Serial.println(F("Successfully created CNT_Sensors!"));
        resource_sum ++;

        fflag = post("/TinyIoT/TinyFarm/Sensors", "CNT", "Temperature", ""); 
        if (fflag == 1){
          Serial.println(F("Successfully created CNT_Temperature!"));
          resource_sum ++;
          fflag = 0;
        }
        else{
          Serial.println(F("Failed to create CNT_Temperature!"));
          fflag = 0;
        }

        fflag = post("/TinyIoT/TinyFarm/Sensors", "CNT", "humidity", "");
        if (fflag == 1){
          Serial.println(F("Successfully created CNT_humidity!"));
          resource_sum ++;
          fflag = 0;
        }
        else{
          Serial.println(F("Failed to create CNT_humidity!"));
          fflag = 0;
        }
       

        fflag = post("/TinyIoT/TinyFarm/Sensors", "CNT", "CO2", "");
        if (fflag == 1){
          Serial.println(F("Successfully created CNT_CO2!"));
          resource_sum ++;
          fflag = 0;
        }
        else{
          Serial.println(F("Failed to create CNT_CO2!"));
          fflag = 0;
        }

      }
      else{
         Serial.println(F("Failed to create CNT_Sensors!"));
      }

      int state_CNT2 = post("/TinyIoT/TinyFarm", "CNT", "Actuators", "");
      if(state_CNT2 == 1){
        Serial.println(F("Successfully created CNT_Actuators!"));
        resource_sum ++;

        fflag = post("/TinyIoT/TinyFarm/Actuators", "CNT", "LED", ""); 
        if (fflag == 1){
          Serial.println(F("Successfully created CNT_LED!"));
          resource_sum ++;
          fflag = 0;
        }
        else{
          Serial.println(F("Failed to create CNT_LED!"));
          fflag = 0;
        }

        fflag = post("/TinyIoT/TinyFarm/Actuators", "CNT", "Water", "");
        if (fflag == 1){
          Serial.println(F("Successfully created CNT_Water!"));
          resource_sum ++;
          fflag = 0;
        }
        else{
          Serial.println(F("Failed to create CNT_Water!"));
          fflag = 0;
        }

      }
      else{
         Serial.println(F("Failed to create CNT_Actuators!"));
      }

    } 
    else if(state == 0){
      Serial.println(F("Invalid Header or URI"));
    } 
    else {
      Serial.println(F("Wifi not connected!"));
    }

  } 
  else{
    Serial.println(F("Wifi not connected!"));
  }
  
  Serial.println(F("Setup completed!"));
}




DHT dht(DHTPIN, DHTTYPE);
Servo servo; 
LiquidCrystal_I2C lcd(0x27, 16, 2);


// 액정 화면 출력 함수
// 스마트팜 농장의 LCD 화면에 현재 센서 값이 출력되게함
void printLCD(int col, int row , char *str) {
    for(int i=0 ; i < strlen(str) ; i++){
      lcd.setCursor(col+i , row);
      lcd.print(str[i]);
    }}


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
  Serial.begin(9600);
  Serial1.begin(9600);
  Serial2.begin(9600); 
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

  setDevice();
  if(resource_sum == RESOURCE_CREATED_SUM) {
    Serial.println("SmartFarm Succesfully Registered");
  }
  else{
    Serial.println("Failed to register SmartFarm");
  }
}

void loop() {
  unsigned long now = millis();

  // ====== DHT (습도/온도): 한 번만 읽어서 두 항목에 공유 ======
  bool needHum  = (now - lastHumPost  >= HUM_POST_MS);
  bool needTemp = (now - lastTempPost >= TEMP_POST_MS);

  if (needHum || needTemp) {
    float h = dht.readHumidity();        // 보통 readHumidity()
    float t = dht.readTemperature();     // ℃

    if (needHum && !isnan(h)) {
      String sH = String(h);
      post("/TinyIoT/TinyFarm/Sensors/humidity", "CIN", "", sH);
      lastHumPost = now;
    }
    if (needTemp && !isnan(t)) {
      String sT = String(t);
      post("/TinyIoT/TinyFarm/Sensors/Temperature", "CIN", "", sT);
      lastTempPost = now;
    }
  }

  // ====== 토양수분 ======
  if (now - lastSoilPost >= SOIL_POST_MS) {
    int waterValue = analogRead(1);
    waterValue = (10230 - waterValue * 10) / 100;
    char wat_buffer[16] = {0};
    itoa(waterValue, wat_buffer, 10);
    post("/TinyIoT/TinyFarm/Sensors/Soil", "CIN", "", wat_buffer);
    lastSoilPost = now;
  }

  // ====== CO2 ======
  if (now - lastCO2Post >= CO2_POST_MS) {
    float EMF  = analogRead(EMF_pin);
    delay(1);
    EMF = EMF / (ADCResol - 1) * ADCvolt / 6 * 1000;

    float THER = analogRead(THER_pin);
    delay(1);
    THER = 1 / (C1 + C2 * log((Resist_0 * THER)/(ADCResol - THER))
                   + C3 * pow(log((Resist_0 * THER)/(ADCResol - THER)), 3)) - 273.15;

    int co2_ppm = RX9.cal_co2(EMF, THER) / 10;
    char co2_buffer[16] = {0};
    itoa(co2_ppm, co2_buffer, 10);
    post("/TinyIoT/TinyFarm/Sensors/CO2", "CIN", "", co2_buffer);

    lastCO2Post = now;
  }

  if (now - lastCmdPoll >= CMD_POLL_MS) {
    // 네트워크로부터 들어온 명령을 처리 (창문, led, 환풍기, 물펌프)
      //ON / OFF 함수는 모두 대문자로 입력되어야 함
    String cmd1 = getCommandFromTinyIoT("/TinyIoT/TinyFarm/Actuators/Door/la"); // 창문 명령을 처리하는 함수

    if (cmd1.length() > 0) {

      const char* nData = cmd1.c_str();

      if(memcmp(nData, "ON", 2) == 0) {
        angle = (nData[0] == 'O' && nData[1] == 'N') ? 10 : 80;
        servo.attach(SERVOPIN);
        servo.write(angle); 
        delay(500);
        servo.detach();
        Serial.print("[창문 제어] angle=");
        Serial.println(angle);
      }

      if(memcmp(nData, "OFF", 3) == 0) {
        angle = (nData[0] == 'O' && nData[1] == 'N') ? 10 : 80;
        servo.attach(SERVOPIN);
        servo.write(angle); 
        delay(500);
        servo.detach();
        Serial.print("[창문 제어] angle=");
        Serial.println(angle);
      }

    }


    String cmd2 = getCommandFromTinyIoT("/TinyIoT/TinyFarm/Actuators/Fan/la"); // 환풍기 명령을 처리하는 함수

    if (cmd2.length() > 0){

      const char* nData = cmd2.c_str();

      if(memcmp(nData, "ON", 2) == 0) {

        digitalWrite(FAN_PIN, (nData[0] == 'O' && nData[1] == 'N') ? HIGH : LOW);
        Serial.print("[팬 제어] FAN=");
        Serial.println((nData[0] == 'O' && nData[1] == 'N') ? 1 : 0);
      }
      else if(memcmp(nData, "OFF", 3) == 0) {

        digitalWrite(FAN_PIN, (nData[0] == 'O' && nData[1] == 'N') ? HIGH : LOW);
        Serial.print("[팬 제어] FAN=");
        Serial.println((nData[0] == 'O' && nData[1] == 'N') ? 1 : 0);
      }
    }


    String cmd3 = getCommandFromTinyIoT("/TinyIoT/TinyFarm/Actuators/LED/la"); // LED 밝기 명령을 제어하는 함수

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

  // 자동급수 명령은 물높이와 물펌프 세팅을 정확히 하고난 다음에 활성화하기
    // String cmd4 = getCommandFromTinyIoT("/TinyIoT/TinyFarm/Actuators/Water/la"); // 자동급수 명령을 처리하는 함수

      
    // if (cmd4.length() > 0){ 

    //   const char* nData = cmd4.c_str();

    //   if(memcmp(nData, "ON", 2) == 0) {
    //     digitalWrite(WATER_PUMP_PIN, (nData[0] == 'O' && nData[1] == 'N') ? HIGH : LOW);
    //     Serial.print("[펌프 제어] WATER=");
    //     Serial.println((nData[0] == 'O' && nData[1] == 'N') ? 1 : 0);
    //   }

    //   if(memcmp(nData, "OFF", 3) == 0) {
    //     digitalWrite(WATER_PUMP_PIN, (nData[0] == 'O' && nData[1] == 'N') ? HIGH : LOW);
    //     Serial.print("[펌프 제어] WATER=");
    //     Serial.println((nData[0] == 'O' && nData[1] == 'N') ? 1 : 0);
    //   }

    // }
    lastCmdPoll = now;
  }


  while (Serial1.available() > 0) {
      
    char ch = Serial1.read();
      rData[rPos] = ch; rPos += 1;
      Serial.print(ch);
  
      if(ch == '\n')
      {         
        #if DEBUG
        Serial.print("rPos=");
        Serial.print(rPos);
        Serial.print(" ");
        Serial.println(rData);
        #endif
        
        if(memcmp(rData, "C_S-", 4) == 0) //창문 열고 닫는 함수 (10' , 80')
        {
          if(rData[4] == '0') angle = 10;
          else angle = 80;
          servo.attach(SERVOPIN);
          servo.write(angle); 
          delay(500);
          servo.detach();
          #if DEBUG
          Serial.print("server_f_MOTOR=");
          Serial.println(angle);
          #endif
        }
        
        if(memcmp(rData, "C_F-", 4) == 0) // 환풍기 실행/정지 관련 제어 함수
        {
          if(rData[4] == '0') digitalWrite(FAN_PIN, 0);
          else digitalWrite(FAN_PIN, 1);
          #if DEBUG
          Serial.print("FAN=");
          Serial.println(rData[4]);
          #endif
        }
  
        if(memcmp(rData, "C_L-", 4) == 0) // LED 밝기 제어하는 함수
        {
        int light = atoi(rData+4);
        lightoutput = light;
        analogWrite(LIGHTPIN, (int)(25 * light));
          #if DEBUG
          Serial.print("LIGHT=");
          Serial.println(25 * light); // light);
          #endif
        }
  
        // if(memcmp(rData, "C_W-", 4) == 0) // 자동급수 펌프 제어하는 함수
        // {
        //   if(rData[4] == '0') digitalWrite(WATER_PUMP_PIN, 0);
        //   else digitalWrite(WATER_PUMP_PIN, 1);
        //   #if DEBUG
        //   Serial.print("WATER=");
        //   Serial.println(rData[4]);
        //   #endif
        // }
        
        rPos = 0;
        memset(rData, 0x00,32);
        break;
      }
  }

  // 여기는 더 이상 delay(10000) 사용하지 않습니다.
  // (ESP 계열이면) watchdog/스케줄링을 위해 살짝 양보
  delay(1); // 또는 yield();
}