//20250725 아두이노 업로드 할 코드
//이산화탄소코드 포함
#include <Servo.h>
#include "DHT.h"

// ======== [ADDED] Profile-based configuration ========
struct DeviceProfile {
  const char* name;
  uint32_t humPeriodMs;
  uint32_t tempPeriodMs;
  uint32_t soilPeriodMs;
  uint32_t co2PeriodMs;
  uint32_t cmdPeriodMs;
  const char* basePath;     // e.g. "/TinyIoT/TinyFarm"
  const char* origin;       // X-M2M-Origin
  const char* server;       // optional override
  int        port;          // optional override
};

DeviceProfile PROFILES[] = {
  { "device1", 10000, 10000, 30000, 60000, 5000, "/TinyIoT/TinyFarm", "SArduino1", nullptr, -1 },
  { "device2",  5000,  5000, 15000, 30000, 5000, "/TinyIoT/TinyFarm", "SArduino2", nullptr, -1 },
};
constexpr size_t PROFILE_COUNT = sizeof(PROFILES)/sizeof(PROFILES[0]);
#define DEFAULT_PROFILE "device1"

const DeviceProfile* ACTIVE = nullptr;

uint32_t HUM_POST_MS;
uint32_t TEMP_POST_MS;
uint32_t SOIL_POST_MS;
uint32_t CO2_POST_MS;
uint32_t CMD_POLL_MS;

const DeviceProfile* findProfile(const char* name) {
  for (size_t i=0;i<PROFILE_COUNT;++i) if (strcmp(PROFILES[i].name, name)==0) return &PROFILES[i];
  return nullptr;
}

void applyActiveProfile(const DeviceProfile* p) {
  ACTIVE = p;
  HUM_POST_MS  = p->humPeriodMs;
  TEMP_POST_MS = p->tempPeriodMs;
  SOIL_POST_MS = p->soilPeriodMs;
  CO2_POST_MS  = p->co2PeriodMs;
  CMD_POLL_MS  = p->cmdPeriodMs;
  Serial.print(F("[PROFILE] ")); Serial.print(p->name);
  Serial.print(F(" basePath=")); Serial.print(p->basePath);
  Serial.print(F(" origin=")); Serial.println(p->origin);
}

String applyBasePath(const char* path) {
  String s(path);
  const char* oldBase = "/TinyIoT/TinyFarm";
  if (ACTIVE && s.startsWith(oldBase)) {
    String rest = s.substring(strlen(oldBase));
    return String(ACTIVE->basePath) + rest;
  }
  return s;
}

void handleSerialCommand() {
  if (!Serial.available()) return;
  String line = Serial.readStringUntil('\\n'); line.trim();
  if (line.startsWith("profile ")) {
    String name = line.substring(8); name.trim();
    auto p = findProfile(name.c_str());
    if (p) { applyActiveProfile(p); Serial.println(F("[CMD] profile switched")); }
    else   { Serial.println(F("[CMD] unknown profile")); }
  } else if (line == "show") {
    if (ACTIVE) {
      Serial.print(F("[INFO] active=")); Serial.print(ACTIVE->name);
      Serial.print(F(", hum=")); Serial.print(HUM_POST_MS);
      Serial.print(F(", temp=")); Serial.print(TEMP_POST_MS);
      Serial.print(F(", basePath=")); Serial.println(ACTIVE->basePath);
    } else {
      Serial.println(F("[INFO] no active profile"));
    }
  }
}
// ======== [END ADDED] Profile-based configuration ========
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


// 마지막 전송 시각
unsigned long lastHumPost  = 0;
unsigned long lastTempPost = 0;
unsigned long lastSoilPost = 0;
unsigned long lastCO2Post  = 0;

// TinyIoT 원격명령 주기
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
HttpClient http(client, "203.250.148.89", 3000);

String server = "203.250.148.89";
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
    // 요청 전송
    String request = "GET " + path + " HTTP/1.1\r\n";
    request += "Host: " + server + ":" + port + "\r\n";
    request += "X-M2M-RI: retrieve\r\n";
    request += "X-M2M-Rvi: 2a\r\n";
    request += "X-M2M-Origin: CAdmin\r\n";
    request += "Accept: application/json\r\n";
    request += "Connection: close\r\n\r\n";
    client.print(request);

    // 응답 읽기
    bool inBody = false;
    unsigned long timeout = millis();

    while (millis() - timeout < 3000) { // 최대 3초 대기
      while (client.available()) {
        String line = client.readStringUntil('\n');
        line.trim();  // \r, 공백 제거

        // --- 디버깅 ---
        Serial.print("LINE: [");
        Serial.print(line);
        Serial.println("]");

        if (!inBody) {
          // 빈 줄("") 또는 "\r" → 헤더 끝
          if (line.length() == 0) {
            inBody = true;
          }
        } else {
          payload += line;
        }
        timeout = millis(); // 데이터 들어오면 타이머 리셋
      }
    }
    client.stop();
  } else {
    Serial.println("TinyIoT 연결 실패");
    return "";
  }

  // --- RAW 출력 ---
  Serial.println("=== RAW PAYLOAD START ===");
  Serial.println(payload);
  Serial.println("=== RAW PAYLOAD END ===");

  // JSON 파싱
  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload);

  if (error) {
    Serial.print("JSON 파싱 실패: ");
    Serial.println(error.f_str());
    return "";
  }

  if (doc["m2m:cin"]["con"]) {
    return String((const char*)doc["m2m:cin"]["con"]);
  }

  Serial.println("con 필드 없음");
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
  int status = get(applyBasePath("/TinyIoT/TinyFarm"));
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

      int state_CNT1 = post(applyBasePath("/TinyIoT/TinyFarm"), "CNT", "Sensors", "");
      if(state_CNT1 == 1){
        Serial.println(F("Successfully created CNT_Sensors!"));
        resource_sum ++;

        fflag = post(applyBasePath("/TinyIoT/TinyFarm/Sensors"), "CNT", "Temperature", ""); 
        if (fflag == 1){
          Serial.println(F("Successfully created CNT_Temperature!"));
          resource_sum ++;
          fflag = 0;
        }
        else{
          Serial.println(F("Failed to create CNT_Temperature!"));
          fflag = 0;
        }

        fflag = post(applyBasePath("/TinyIoT/TinyFarm/Sensors"), "CNT", "humidity", "");
        if (fflag == 1){
          Serial.println(F("Successfully created CNT_humidity!"));
          resource_sum ++;
          fflag = 0;
        }
        else{
          Serial.println(F("Failed to create CNT_humidity!"));
          fflag = 0;
        }
       

        fflag = post(applyBasePath("/TinyIoT/TinyFarm/Sensors"), "CNT", "CO2", "");
        if (fflag == 1){
          Serial.println(F("Successfully created CNT_CO2!"));
          resource_sum ++;
          fflag = 0;
        }
        else{
          Serial.println(F("Failed to create CNT_CO2!"));
          fflag = 0;
        }

        fflag = post("/TinyIoT/TinyFarm/Sensors", "CNT", "soil", "");         //soil(cnt) 생성
        if (fflag == 1){
          Serial.println(F("Successfully created CNT_soil!"));
          resource_sum ++;
          fflag = 0;
        }
        else{
          Serial.println(F("Failed to create CNT_soil!"));
          fflag = 0;
        }

      }
      else{
         Serial.println(F("Failed to create CNT_Sensors!"));
      }

      int state_CNT2 = post(applyBasePath("/TinyIoT/TinyFarm"), "CNT", "Actuators", "");
      if(state_CNT2 == 1){
        Serial.println(F("Successfully created CNT_Actuators!"));
        resource_sum ++;

        fflag = post("/TinyIoT/TinyFarm/Actuators", "CNT", "Door", "");       //Door(cnt) 생성 
        if (fflag == 1){
          Serial.println(F("Successfully created CNT_Door!"));
          resource_sum ++;
          fflag = 0;
        }
        else{
          Serial.println(F("Failed to create CNT_Door!"));
          fflag = 0;
        }

        fflag = post("/TinyIoT/TinyFarm/Actuators", "CNT", "Fan", "");       //Fan(cnt) 생성 
        if (fflag == 1){
          Serial.println(F("Successfully created CNT_Fan!"));
          resource_sum ++;
          fflag = 0;
        }
        else{
          Serial.println(F("Failed to create CNT_Fan!"));
          fflag = 0;
        }

        fflag = post("/TinyIoT/TinyFarm/Actuators", "CNT", "LED", "");          //LED(cnt) 생성
        if (fflag == 1){
          Serial.println(F("Successfully created CNT_LED!"));
          resource_sum ++;
          fflag = 0;
        }
        else{
          Serial.println(F("Failed to create CNT_LED!"));
          fflag = 0;
        }

        fflag = post(applyBasePath("/TinyIoT/TinyFarm/Actuators"), "CNT", "Water", "");
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
// [ADDED] initialize active profile
{
  const DeviceProfile* p = findProfile(DEFAULT_PROFILE);
  if (p) applyActiveProfile(p);
  else applyActiveProfile(&PROFILES[0]);
}

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
  post("/TinyIoT/TinyFarm/Actuators/Door", "CIN", "", "OFF");
  post("/TinyIoT/TinyFarm/Actuators/Fan", "CIN", "", "OFF");
  post("/TinyIoT/TinyFarm/Actuators/LED", "CIN", "", "0");
  //post("/TinyIoT/TinyFarm/Actuators/Water", "CNT", "Door", "");
  if(resource_sum == RESOURCE_CREATED_SUM) {
    Serial.println("SmartFarm Succesfully Registered");
  }
  else{
    Serial.println("Failed to register SmartFarm");
  }
}

void loop() {
  handleSerialCommand(); // [ADDED] runtime profile switch

  unsigned long now = millis();

  int tem;
  int hum;
  int co;

  // ====== DHT (습도/온도): 한 번만 읽어서 두 항목에 공유 ======
  bool needHum  = (now - lastHumPost  >= HUM_POST_MS);
  bool needTemp = (now - lastTempPost >= TEMP_POST_MS);

  if (needHum || needTemp) {
    float h = dht.readHumidity();        // 보통 readHumidity()
    float t = dht.readTemperature();     // ℃
    tem = (int)t;
    hum = (int)h;


    if (needHum && !isnan(h)) {
      String sH = String(h);
      post(applyBasePath("/TinyIoT/TinyFarm/Sensors/humidity"), "CIN", "", sH);
      lastHumPost = now;
    }
    if (needTemp && !isnan(t)) {
      String sT = String(t);
      post(applyBasePath("/TinyIoT/TinyFarm/Sensors/Temperature"), "CIN", "", sT);
      lastTempPost = now;
    }
  }

  // ====== 토양수분 ======
  if (now - lastSoilPost >= SOIL_POST_MS) {
    int waterValue = analogRead(1);
    waterValue = (10230 - waterValue * 10) / 100;
    char wat_buffer[16] = {0};
    itoa(waterValue, wat_buffer, 10);
    post(applyBasePath("/TinyIoT/TinyFarm/Sensors/Soil"), "CIN", "", wat_buffer);
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
    int co = co2_ppm;
    char co2_buffer[16] = {0};
    itoa(co2_ppm, co2_buffer, 10);
    post(applyBasePath("/TinyIoT/TinyFarm/Sensors/CO2"), "CIN", "", co2_buffer);

    lastCO2Post = now;
  }

  if (now - lastCmdPoll >= CMD_POLL_MS) {
    // 네트워크로부터 들어온 명령을 처리 (창문, led, 환풍기, 물펌프)
      //ON / OFF 함수는 모두 대문자로 입력되어야 함
    String cmd1 = getCommandFromTinyIoT("/TinyIoT/TinyFarm/Actuators/Door/la"); // 창문 명령을 처리하는 함수

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


    String cmd2 = getCommandFromTinyIoT("/TinyIoT/TinyFarm/Actuators/Fan/la"); // 환풍기 명령을 처리하는 함수
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



      //센서값 LCD 출력


      lcd.clear();
      displayToggle = !displayToggle;
      if(displayToggle == 1) {
        memset(sData, 0x00, 64);
        sprintf(sData, "temp %02dC humi %02d%%", tem,
        hum);
        printLCD(0, 0, sData);
        memset(sData, 0x00, 64);
        sprintf(sData, "co2%-04d soil%-04d", co, waterValue);
        printLCD(0, 1, sData);
      }
      else {
        memset(sData, 0x00, 64);
        sprintf(sData, "temp %02dC humi %02d%%", tem, 
        hum);
        printLCD(0, 0, sData);
        memset(sData, 0x00, 64);
        printLCD(0, 1, sData);
      }

      sprintf(sData, "{ \"temp\":%02d,\"humidity\":%02d,\"co2\":%-04d,\"water\":%-04d }", 
      tem, hum, co, waterValue);
    
  
      Serial1.println(sData);
  // 여기는 더 이상 delay(10000) 사용하지 않습니다.
  // (ESP 계열이면) watchdog/스케줄링을 위해 살짝 양보
  delay(1); // 또는 yield();
}
