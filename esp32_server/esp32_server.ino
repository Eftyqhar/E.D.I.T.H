#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

const char* ssid = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

const char* ap_ssid = "SmartHome_Offline"; 
const char* ap_pass = "12345678"; 

#define FIREBASE_HOST "Your_Firebase_Project_ID.firebaseio.com" 
#define FIREBASE_API_KEY "Your_Firebase_API_Key" 

FirebaseData fbdo; 
FirebaseAuth auth;
FirebaseConfig config;
bool firebaseInitialized = false; 

WebServer server(80);

const int relay1 = 15;
const int relay2 = 16;
const int relay3 = 17;
const int relay4 = 18;

#define DHTPIN 4    
#define DHTTYPE DHT11 
DHT dht(DHTPIN, DHTTYPE);

// const int currentPin = 9;

bool relay1State = false;
bool relay2State = false;
bool relay3State = false;
bool relay4State = false;

float temperature = 0.0;
float humidity = 0.0;
float currentVal = 0.0;
float powerVal = 0.0;
// float currentCalibrationOffset = 0.0; 

unsigned long previousMillis = 0;
const long interval = 2000; 

/* === Current Sensor er Function gulo comment kora holo ===
float getVPP() {
  float result;
  int readValue;
  int maxValue = 0;
  int minValue = 4095;
  uint32_t start_time = millis();
  
  while((millis() - start_time) < 25) {
     readValue = analogRead(currentPin);
     if (readValue > maxValue) {
         maxValue = readValue;
     }
     if (readValue < minValue) {
         minValue = readValue;
     }
  }
  
  result = ((maxValue - minValue) * 3.3) / 4095.0;
  return result;
}

void calibrateACS712() {
  Serial.println("Calibrating Current Sensor... Keep all loads OFF.");
  float tempVpp = 0;
  for(int i=0; i<10; i++) {
     tempVpp += getVPP();
     delay(10);
  }
  float avgVpp = tempVpp / 10.0;
  float VRMS = (avgVpp / 2.0) * 0.707;
  currentCalibrationOffset = VRMS / 0.1221;
  Serial.print("Calibration Offset (Noise Base): ");
  Serial.println(currentCalibrationOffset);
}
========================================================= */

String getWebPage() {
  String html = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">";
  html += "<title>Smart Home</title>";
  html += "<style>body{font-family:Arial; text-align:center; background:#0f172a; color:white;}";
  html += ".btn{padding:15px 30px; font-size:20px; margin:10px; border:none; border-radius:5px; cursor:pointer;}";
  html += ".btn-on{background-color:#10b981; color:white;}";
  html += ".btn-off{background-color:#ef4444; color:white;}";
  html += ".card{background:#1e293b; padding:20px; margin:20px auto; width:80%; max-width:400px; border-radius:10px;}</style></head><body>";
  
  html += "<h2>Smart Home Dashboard</h2>";
  html += "<p style='color:#3b82f6;' id='mode-status'>Mode: Loading...</p>"; 
  
  html += "<div class='card'>";
  html += "<h3>Sensor Data</h3>";
  html += "<p>Temperature: <b id='temp-val'>" + String(temperature) + " &deg;C</b></p>";
  html += "<p>Humidity: <b id='hum-val'>" + String(humidity) + " %</b></p>";
  html += "<p>Current: <b id='curr-val'>" + String(currentVal) + " A</b></p>";
  html += "<p>Power: <b id='pow-val'>" + String(powerVal) + " W</b></p>";
  html += "</div>";

  html += "<div class='card'><h3>Appliance Control</h3>";
  
  html += "<p>Device 1 (Light): <a href=\"/toggle1\"><button class=\"btn " + String(relay1State ? "btn-on" : "btn-off") + "\">" + String(relay1State ? "ON" : "OFF") + "</button></a></p>";
  html += "<p>Device 2 (Fan): <a href=\"/toggle2\"><button class=\"btn " + String(relay2State ? "btn-on" : "btn-off") + "\">" + String(relay2State ? "ON" : "OFF") + "</button></a></p>";
  html += "<p>Device 3 (TV): <a href=\"/toggle3\"><button class=\"btn " + String(relay3State ? "btn-on" : "btn-off") + "\">" + String(relay3State ? "ON" : "OFF") + "</button></a></p>";
  html += "<p>Device 4 (Pump): <a href=\"/toggle4\"><button class=\"btn " + String(relay4State ? "btn-on" : "btn-off") + "\">" + String(relay4State ? "ON" : "OFF") + "</button></a></p>";
  
  html += "</div>";

  html += "<script>";
  html += "setInterval(function(){";
  html += "fetch('/api/data').then(response => response.json()).then(data => {";
  html += "document.getElementById('temp-val').innerHTML = data.temperature.toFixed(1) + ' &deg;C';";
  html += "document.getElementById('hum-val').innerHTML = data.humidity.toFixed(1) + ' %';";
  html += "document.getElementById('curr-val').innerHTML = data.current.toFixed(2) + ' A';";
  html += "document.getElementById('pow-val').innerHTML = data.power.toFixed(1) + ' W';";
  html += "document.getElementById('mode-status').innerHTML = 'Mode: ' + data.mode;";
  html += "});";
  html += "}, 2000);"; 
  html += "</script>";

  html += "</body></html>";
  return html;
}

void setup() {
  Serial.begin(115200);

  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);
  pinMode(relay4, OUTPUT);

  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
  digitalWrite(relay3, HIGH);
  digitalWrite(relay4, HIGH);

  dht.begin();
  
  // calibrateACS712(); 

  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA); 
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if(WiFi.status() == WL_CONNECTED){
    Serial.println("\nWiFi Connected!");
    Serial.print("Local IP Address: ");
    Serial.println(WiFi.localIP()); 
    
    config.api_key = FIREBASE_API_KEY;
    config.database_url = FIREBASE_HOST;
    config.signer.test_mode = true; 
    
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    firebaseInitialized = true;
    Serial.println("Firebase Connected!");
  } else {
    Serial.println("\nWiFi Connection Failed! Starting Access Point (Offline Mode)...");
    
    WiFi.mode(WIFI_AP_STA); 
    WiFi.softAP(ap_ssid, ap_pass); 
    
    Serial.print("Access Point Created. Connect to WiFi: ");
    Serial.println(ap_ssid);
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP()); 
  }

  server.on("/api/data", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*"); 
    
    String currentMode = (WiFi.status() == WL_CONNECTED) ? "Online (Cloud)" : "Offline (Local AP)";
    
    String json = "{";
    json += "\"temperature\":" + String(temperature) + ",";
    json += "\"humidity\":" + String(humidity) + ",";
    json += "\"current\":" + String(currentVal) + ",";
    json += "\"power\":" + String(powerVal) + ",";
    json += "\"mode\":\"" + currentMode + "\",";
    json += "\"relay1\":" + String(relay1State ? "true" : "false") + ",";
    json += "\"relay2\":" + String(relay2State ? "true" : "false") + ",";
    json += "\"relay3\":" + String(relay3State ? "true" : "false") + ",";
    json += "\"relay4\":" + String(relay4State ? "true" : "false");
    json += "}";
    
    server.send(200, "application/json", json);
  });

  server.on("/api/toggle", HTTP_GET, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*"); 
    
    if (server.hasArg("relay")) {
      String relayNum = server.arg("relay");
      if (relayNum == "1") { relay1State = !relay1State; digitalWrite(relay1, relay1State ? LOW : HIGH); if(firebaseInitialized) Firebase.RTDB.setBool(&fbdo, "/SmartHome/Relay1", relay1State); }
      else if (relayNum == "2") { relay2State = !relay2State; digitalWrite(relay2, relay2State ? LOW : HIGH); if(firebaseInitialized) Firebase.RTDB.setBool(&fbdo, "/SmartHome/Relay2", relay2State); }
      else if (relayNum == "3") { relay3State = !relay3State; digitalWrite(relay3, relay3State ? LOW : HIGH); if(firebaseInitialized) Firebase.RTDB.setBool(&fbdo, "/SmartHome/Relay3", relay3State); }
      else if (relayNum == "4") { relay4State = !relay4State; digitalWrite(relay4, relay4State ? LOW : HIGH); if(firebaseInitialized) Firebase.RTDB.setBool(&fbdo, "/SmartHome/Relay4", relay4State); }
      
      server.send(200, "application/json", "{\"status\":\"success\"}");
    } else {
      server.send(400, "application/json", "{\"status\":\"error\", \"message\":\"relay parameter missing\"}");
    }
  });

  server.on("/", []() {
    server.send(200, "text/html", getWebPage());
  });

  server.on("/toggle1", []() {
    relay1State = !relay1State;
    digitalWrite(relay1, relay1State ? LOW : HIGH); 
    if(firebaseInitialized) Firebase.RTDB.setBool(&fbdo, "/SmartHome/Relay1", relay1State);
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });

  server.on("/toggle2", []() {
    relay2State = !relay2State;
    digitalWrite(relay2, relay2State ? LOW : HIGH);
    if(firebaseInitialized) Firebase.RTDB.setBool(&fbdo, "/SmartHome/Relay2", relay2State);
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });

  server.on("/toggle3", []() {
    relay3State = !relay3State;
    digitalWrite(relay3, relay3State ? LOW : HIGH);
    if(firebaseInitialized) Firebase.RTDB.setBool(&fbdo, "/SmartHome/Relay3", relay3State);
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });

  server.on("/toggle4", []() {
    relay4State = !relay4State;
    digitalWrite(relay4, relay4State ? LOW : HIGH);
    if(firebaseInitialized) Firebase.RTDB.setBool(&fbdo, "/SmartHome/Relay4", relay4State);
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  unsigned long currentMillis = millis();

  server.handleClient();

  static unsigned long previousWifiMillis = 0;
  const long wifiReconnectInterval = 30000; 

  if (WiFi.status() != WL_CONNECTED) {
    if (currentMillis - previousWifiMillis >= wifiReconnectInterval) {
      previousWifiMillis = currentMillis;
      Serial.println("WiFi Disconnected. Attempting to reconnect...");
      WiFi.disconnect();
      WiFi.mode(WIFI_AP_STA); 
      WiFi.begin(ssid, password);
    }
  } else {
    if (!firebaseInitialized) {
      Serial.println("WiFi Reconnected! Initializing Firebase...");
      config.api_key = FIREBASE_API_KEY;
      config.database_url = FIREBASE_HOST;
      config.signer.test_mode = true;
      
      Firebase.begin(&config, &auth);
      Firebase.reconnectWiFi(true);
      firebaseInitialized = true;
    }
  }

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) temperature = t;
    if (!isnan(h)) humidity = h;

    /* === Loop er vetor Current er hishab comment kora holo ===
    float Vpp = getVPP(); 
    float VRMS = (Vpp / 2.0) * 0.707; 
    
    float rawCurrent = VRMS / 0.1221; 
    
    currentVal = rawCurrent - currentCalibrationOffset;
    
    if(currentVal < 0.05) {
      currentVal = 0.0;
    }
    
    powerVal = 220.0 * currentVal;
    ========================================================= */

    Serial.println("Temp: " + String(temperature) + "C, Hum: " + String(humidity) + "%");
    // Serial.println("Current: " + String(currentVal) + "A, Power: " + String(powerVal) + "W");

    if (WiFi.status() == WL_CONNECTED && firebaseInitialized) {
    
      if (Firebase.RTDB.getBool(&fbdo, "/SmartHome/Relay1")) {
        bool r1 = fbdo.boolData();
        if (r1 != relay1State) { relay1State = r1; digitalWrite(relay1, relay1State ? LOW : HIGH); }
      }
      if (Firebase.RTDB.getBool(&fbdo, "/SmartHome/Relay2")) {
        bool r2 = fbdo.boolData();
        if (r2 != relay2State) { relay2State = r2; digitalWrite(relay2, relay2State ? LOW : HIGH); }
      }
      if (Firebase.RTDB.getBool(&fbdo, "/SmartHome/Relay3")) {
        bool r3 = fbdo.boolData();
        if (r3 != relay3State) { relay3State = r3; digitalWrite(relay3, relay3State ? LOW : HIGH); }
      }
      if (Firebase.RTDB.getBool(&fbdo, "/SmartHome/Relay4")) {
        bool r4 = fbdo.boolData();
        if (r4 != relay4State) { relay4State = r4; digitalWrite(relay4, relay4State ? LOW : HIGH); }
      }

      if (!Firebase.RTDB.setFloat(&fbdo, "/SmartHome/Temperature", temperature)) {
        Serial.println("Firebase Error (Temp): " + fbdo.errorReason());
      }
      if (!Firebase.RTDB.setFloat(&fbdo, "/SmartHome/Humidity", humidity)) {
        Serial.println("Firebase Error (Hum): " + fbdo.errorReason());
      }

      /* === Firebase e Current/Power pathano comment kora holo ===
      Firebase.RTDB.setFloat(&fbdo, "/SmartHome/Current", currentVal);
      Firebase.RTDB.setFloat(&fbdo, "/SmartHome/Power", powerVal);
      ========================================================== */

      Firebase.RTDB.setBool(&fbdo, "/SmartHome/Relay1", relay1State);
      Firebase.RTDB.setBool(&fbdo, "/SmartHome/Relay2", relay2State);
      Firebase.RTDB.setBool(&fbdo, "/SmartHome/Relay3", relay3State);
      Firebase.RTDB.setBool(&fbdo, "/SmartHome/Relay4", relay4State);
    }
  }
}
