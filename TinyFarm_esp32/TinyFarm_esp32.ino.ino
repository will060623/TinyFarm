#include <SoftwareSerial.h>
#include <WiFi.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>


SoftwareSerial Mega2560(16,17);   //Maga2560 Serial pin(RX,TX)


int Humidity = 0;
int Temperature = 0;
int CO2 = 0;
int Soil = 0;



TaskHandle_t Task1;     //task setting
TaskHandle_t Task2;
TaskHandle_t Task3;
TaskHandle_t Task4;


String server = "127.0.0.1";      //your TinyIoT address
int port = 3000;                  //port

WiFiClient client;
HttpClient http(client, server , port);

const char* ssid = "wifiname";         //wifi name
const char* password = "password";       //password
 

int post(String path, String contentType, String name, String content){     //Post to TinyIoT
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



String serializeJsonBody(String contentType, String name, String content){        //make Json body
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

String unsignedToString(unsigned int value) {
    String result;
    do {
        result = char('0' + (value % 10)) + result;
        value /= 10;
    } while (value > 0);
    return result;
}




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

void setup() {                  // Connect wifi
  Serial.begin(115200);
  Mega2560.begin(9600);

  WiFi.begin(ssid, password);   
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //task
  xTaskCreatePinnedToCore(PostHum,"Task1", 12288, NULL, 1, &Task1, 0);
  xTaskCreatePinnedToCore(PostTemp,"Task2", 12288, NULL, 1, &Task2, 0);
  xTaskCreatePinnedToCore(PostCO2,"Task3", 12288, NULL, 1, &Task3, 0);
  xTaskCreatePinnedToCore(PostSoil,"Task4", 12288, NULL, 1, &Task4, 0);            

}

void loop() {
  if (Mega2560.available()) {                 //Get sensor value from Mega2560
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
