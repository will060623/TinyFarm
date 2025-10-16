#include <SoftwareSerial.h>
#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>


SoftwareSerial Mega2560(16,17);   //Mega와 통신하는 핀


int Humidity = 0;
int Temperature = 0;
int CO2 = 0;
int Soil = 0;



TaskHandle_t Task1;     //task 설정
TaskHandle_t Task2;
TaskHandle_t Task3;
TaskHandle_t Task4;
TaskHandle_t Task5;
TaskHandle_t Task6;
TaskHandle_t Task7;
TaskHandle_t Task8;


String server = "203.250.148.89";
int port = 3000;

WiFiClient client;
HttpClient http(client, server , port);

const char* ssid = "sejong-guest";
const char* password = "0234083114";

//-------------------------Comunication function--------------------------
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
  WiFiClient client;

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
String unsignedToString(unsigned int value) {
    String result;
    do {
        result = char('0' + (value % 10)) + result;
        value /= 10;
    } while (value > 0);
    return result;
}
//-------------------------Comunication function--------------------------



void PostHum(void *pvParameters){
    for(;;){
      Serial.print("tsk1"); 
      String H = String(Humidity);
      post("/TinyIoT/TinyFarm/Sensors/Humidity", "CIN", "", H);     
      vTaskDelay(pdMS_TO_TICKS(10000));
    } 

}

void PostTemp(void *pvParameters){
    for(;;) {
      Serial.print("tsk2");      
      String T = String(Temperature);
      post("/TinyIoT/TinyFarm/Sensors/Temperature", "CIN", "", T);     
      vTaskDelay(pdMS_TO_TICKS(10000));
    } 

}

void PostCO2(void *pvParameters){
    for(;;) {
      Serial.print("tsk3");      
      String C = String(CO2);
      post("/TinyIoT/TinyFarm/Sensors/CO2", "CIN", "", C);     
      vTaskDelay(pdMS_TO_TICKS(10000));
    } 

}

void PostSoil(void *pvParameters){ 
    for(;;) {
      Serial.print("tsk4");      
      String S = String(Soil);
      post("/TinyIoT/TinyFarm/Sensors/Soil", "CIN", "", S);     
      vTaskDelay(pdMS_TO_TICKS(10000));
    } 

}
/*
void GetDoor(void *pvParameters) {
    for(;;){        
      String cmd1 = getCommandFromTinyIoT("/TinyIoT/TinyFarm/Actuators/Door/la"); 
      Serial.print("tsk5 :");      
      Serial.println(cmd1);
      Mega2560.print("Door :");
      Mega2560.println(cmd1);
      vTaskDelay(pdMS_TO_TICKS(10000));       
    }
}

void GetFan(void *pvParameters){ // LED 밝기 명령을 제어하는 함수
    for(;;){      
      String cmd2 = getCommandFromTinyIoT("/TinyIoT/TinyFarm/Actuators/Fan/la");     
      Serial.print("tsk6");      
      Serial.println(cmd2);       
      Mega2560.print("Fan :");
      Mega2560.println(cmd2);
      vTaskDelay(pdMS_TO_TICKS(10000));    
    }
}

void GetLED(void *pvParameters) {
    for(;;){   
      String cmd3 = getCommandFromTinyIoT("/TinyIoT/TinyFarm/Actuators/LED/la");
      Serial.print("tsk7");         
      Serial.println(cmd3);      
      Mega2560.print("LED :");
      Mega2560.print(cmd3);
      vTaskDelay(pdMS_TO_TICKS(10000));          
    }
}

void GetWater(void *pvParameters) {
    for(;;) {     
      String cmd4 = getCommandFromTinyIoT("/TinyIoT/TinyFarm/Actuators/Water/la");
      Serial.print("tsk8");       
      Serial.println(cmd4);      
      Mega2560.print("Water :");
      Mega2560.println(cmd4);
      vTaskDelay(pdMS_TO_TICKS(10000));         
    }
}
*/
void setup() {                  //wifi 연결
  Serial.begin(115200);
  Mega2560.begin(9600);

  WiFi.begin(ssid, password);   // 공유기에 연결
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //task 등록
  xTaskCreatePinnedToCore(PostHum,"Task1", 12288, NULL, 1, &Task1, 0);
  xTaskCreatePinnedToCore(PostTemp,"Task2", 12288, NULL, 1, &Task2, 0);
  xTaskCreatePinnedToCore(PostCO2,"Task3", 12288, NULL, 1, &Task3, 0);
  xTaskCreatePinnedToCore(PostSoil,"Task4", 12288, NULL, 1, &Task4, 0);      

  //xTaskCreatePinnedToCore(GetDoor,"Task5", 12288, NULL, 1 ,&Task5, 1);      
  //xTaskCreatePinnedToCore(GetFan,"Task6", 12288, NULL, 1 ,&Task6, 1); 
  //xTaskCreatePinnedToCore(GetLED,"Task7", 12288, NULL, 1 ,&Task7, 1); 
  //xTaskCreatePinnedToCore(GetWater,"Task8", 12288, NULL, 1 ,&Task8, 1);       

}

void loop() {
  if (Mega2560.available()) {
    String input = Mega2560.readStringUntil('\n'); 
    input.trim();

    int sep = input.indexOf(':');
    if (sep > 0) {
      String key = input.substring(0, sep);
      key.trim();
      String valStr = input.substring(sep + 1);
      valStr.trim();
      int value = valStr.toInt();

      if (key.equals("Hum")) {
        Humidity = value;
      } else if (key.equals("Temp")) {
        Temperature = value;
      } else if (key.equals("CO2")) {
        CO2 = value;
      } else if (key.equals("Soil")) {
        Soil = value;
      }
    }

    // 확인용 출력  
    Serial.print("Hum=");
    Serial.print(Humidity);
    Serial.print(" Temp=");
    Serial.print(Temperature);
    Serial.print(" CO2=");
    Serial.print(CO2);
    Serial.print(" Soil=");
    Serial.println(Soil);
  }

}
