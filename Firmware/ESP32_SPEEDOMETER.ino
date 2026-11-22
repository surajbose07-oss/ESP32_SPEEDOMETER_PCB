#include <WiFi.h> 
#include <WebServer.h> 
#include <TinyGPS++.h> 
#include <Wire.h> 
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h> 
#include <HTTPClient.h> 

// --- REPLACE THESE WITH YOUR MOBILE HOTSPOT DETAILS --- 
const char* WIFI_SSID = "your wifi name"; 
const char* WIFI_PASS = "your wifi password"; 

// --- TELEGRAM NOTIFICATION CREDENTIALS ---
const String BOT_TOKEN = "YOUR_TELEGRAM_BOT_TOKEN";  
const String CHAT_ID = "YOUR_TELEGRAM_CHAT_ID";      

// --- Hardware Layout Dimensions & Configurations --- 
#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_RESET -1 

// Rotary Encoder Hardware Pins (Bottom Row Layout - Boot Safe Mapping) 
#define ROTARY_CLK 23 
#define ROTARY_DT 19 
#define ROTARY_SW 3 

// --- Global Core Object Instances --- 
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); 
WebServer server(80); 
TinyGPSPlus gps; 

// --- Volatile Variables for Interrupt Service Routines (ISRs) --- 
volatile int speedLimitSetting = 60; 
volatile bool encoderChanged = false; 
volatile bool buttonPressed = false; 
volatile unsigned long lastButtonTime = 0; 

// --- Performance Timers & Background Buffers --- 
unsigned long lastOLEDRefresh = 0; 
unsigned long lastNotificationTime = 0;             
const unsigned long alertCooldown = 30000; // 30 seconds notification filter cooldown         

double currentLat = 0.0; 
double currentLng = 0.0; 
double currentSpeed = 0.0; 
double currentAltitude = 0.0; 

// --- Distance Tracking & Toggle State Variables ---
double startLat = 0.0;
double startLng = 0.0;
double distanceMeters = 0.0;
bool isTrackingActive = false;

// Forward declaration of URL Encoder helper
String urlencode(String str);

// --- 1. Rotation Interrupt Logic (Hardware-Level Execution) --- 
void IRAM_ATTR handleEncoderRotation() { 
  int clkState = digitalRead(ROTARY_CLK); 
  int dtState = digitalRead(ROTARY_DT); 
  if (clkState == dtState) { 
    speedLimitSetting++; 
  } else { 
    speedLimitSetting--; 
  } 
  encoderChanged = true; 
} 

// --- 2. Button Press Interrupt Logic (Hardware-Level Execution) --- 
void IRAM_ATTR handleButtonPress() { 
  unsigned long currentTime = millis(); 
  if (currentTime - lastButtonTime > 250) { 
    buttonPressed = true; 
    lastButtonTime = currentTime; 
  } 
} 

// --- 3. Push Notification API Trigger ---
void sendSpeedAlert(double speed, int limit, double lat, double lng) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Corrected target API endpoint
    String url = "https://telegram.org" + BOT_TOKEN + "/sendMessage";
    
    // Format descriptive textual alert body
    String message = "⚠️ [ALERT]: Speed Limit Exceeded!\n";
    message += "Current Speed: " + String(speed, 1) + " km/h\n";
    message += "Set Limit: " + String(limit) + " km/h\n";
    message += "Location: https://google.com" + String(lat, 6) + "," + String(lng, 6);
    
    String postData = "chat_id=" + CHAT_ID + "&text=" + urlencode(message);
    
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    int httpResponseCode = http.POST(postData);
    if (httpResponseCode > 0) {
      Serial.println("Push Notification Successfully Dispatched!");
    } else {
      Serial.print("Error sending notification status: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

// Simple Helper for Telegram character validation compatibility
String urlencode(String str) {
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i=0; i<str.length(); i++){
        c=str.charAt(i);
        if (isalnum(c)){
            encodedString+=c;
        } else {
            code1=(c & 0xf)+'0';
            if ((c & 0xf) >9){ code1=(c & 0xf) - 10 + 'A'; }
            c=(c>>4)&0xf;
            code0=c+'0';
            if (c > 9){ code0=c - 10 + 'A'; }
            encodedString+='%';
            encodedString+=code0;
            encodedString+=code1;
        }
    }
    return encodedString;
}

// --- 4. High-Efficiency Premium Web UI Dashboard Layout --- 
// FIXED: Explicitly cast F() macros to String() to resolve ambiguous operator conversion bugs on ESP32
void handleRootPage() { 
  String html = String(F("<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1.0'>")); 
  html += String(F("<meta http-equiv='refresh' content='2'>")); 
  html += String(F("<title>ESP Dashboard</title><style>")); 
  html += String(F("body{font-family:'Segoe UI',sans-serif;margin:0;padding:20px;background:#0f172a;color:#f8fafc;display:flex;flex-direction:column;align-items:center;}")); 
  html += String(F(".container{width:100%;max-width:500px;} h1{font-size:24px;color:#38bdf8;text-align:center;margin-bottom:5px;}")); 
  html += String(F(".status-bar{display:flex;justify-content:space-between;background:#1e293b;padding:12px;border-radius:8px;margin-bottom:15px;font-size:14px;box-shadow:0 2px 4px rgba(0,0,0,0.2);}")); 
  html += String(F(".grid{display:grid;grid-template-columns:1fr 1fr;gap:12px;}")); 
  html += String(F(".card{background:#1e293b;padding:15px;border-radius:12px;box-shadow:0 4px 6px rgba(0,0,0,0.1);border:1px solid #334155;}")); 
  html += String(F(".full-width{grid-column: span 2;} .label{font-size:12px;color:#94a3b8;text-transform:uppercase;letter-spacing:0.5px;}")); 
  html += String(F(".val{font-size:22px;font-weight:bold;color:#f1f5f9;margin-top:4px;} .unit{font-size:14px;color:#64748b;}")); 
  html += String(F(".alert-box{background:#ef4444;color:#fff;padding:12px;border-radius:8px;font-weight:bold;text-align:center;margin-top:15px;animation:pulse 1.5s infinite;}")); 
  html += String(F("a{color:#38bdf8;text-decoration:none;font-weight:bold;} @keyframes pulse{0%{opacity:0.8;}50%{opacity:1;}100%{opacity:0.8;} }")); 
  html += String(F("</style></head><body><div class='container'>")); 
  
  html += String(F("<h1>Telemetry Control Panel</h1>")); 
  
  // Status ribbon header
  html += String(F("<div class='status-bar'><div>Mode: "));
  if (isTrackingActive) {
    html += String(F("<b style='color:#4ade80;'>RUNNING</b></div>"));
  } else {
    html += String(F("<b style='color:#94a3b8;'>STANDBY</b></div>"));
  }
  html += String(F("<div>Satellites: <b style='color:#38bdf8;'>")) + String(gps.satellites.value()) + String(F("</b></div></div>"));
  
  // Grid layout parameters block
  html += String(F("<div class='grid'>"));
  
  html += String(F("<div class='card'><div class='label'>Current Speed</div><div class='val'>")) + String(currentSpeed, 1) + String(F(" <span class='unit'>km/h</span></div></div>"));
  html += String(F("<div class='card'><div class='label'>Speed Limit</div><div class='val'>")) + String(speedLimitSetting) + String(F(" <span class='unit'>km/h</span></div></div>"));
  
  html += String(F("<div class='card full-width'><div class='label'>Calculated Distance</div><div class='val' style='color:#22c55e;'>")) + String(distanceMeters, 1) + String(F(" <span class='unit'>m</span></div></div>"));
  html += String(F("<div class='card full-width'><div class='label'>Altitude</div><div class='val'>")) + String(currentAltitude, 1) + String(F(" <span class='unit'>m (Sea Level)</span></div></div>"));
  
  html += String(F("<div class='card'><div class='label'>Latitude</div><div class='val' style='font-size:16px;'>")) + String(currentLat, 6) + String(F("</div></div>"));
  html += String(F("<div class='card'><div class='label'>Longitude</div><div class='val' style='font-size:16px;'>")) + String(currentLng, 6) + String(F("</div></div>"));
  
  html += String(F("<div class='card full-width' style='text-align:center;'><a href='https://google.com")) + String(currentLat, 6) + String(F(",")) + String(currentLng, 6) + String(F("' target='_blank'>🗺️ Open Location in Google Maps</a></div>"));
  
  html += String(F("</div>")); // End Grid
  
  if (currentSpeed > speedLimitSetting) { 
    html += String(F("<div class='alert-box'>⚠️ OVER-SPEED WARNING THRESHOLD EXCEEDED</div>")); 
  } 
  
  html += String(F("</div></body></html>")); 
  server.send(200, "text/html", html); 
} 

void setup() { 
  Serial.begin(115200); 
  Serial2.begin(9600, SERIAL_8N1, 16, 17); 
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("OLED Frame Allocation Failed!")); 
    for(;;); 
  } 
  display.clearDisplay(); 
  display.setTextSize(1); 
  display.setTextColor(SSD1306_WHITE); 
  display.setCursor(0, 0); 
  display.println("Connecting to:"); 
  display.println(WIFI_SSID); 
  display.display(); 

  WiFi.mode(WIFI_STA); 
  WiFi.begin(WIFI_SSID, WIFI_PASS); 
  while (WiFi.status() != WL_CONNECTED) { 
    delay(500); 
    Serial.print("."); 
  } 
  
  display.clearDisplay(); 
  display.setCursor(0, 0); 
  display.println("Connected to Hotspot!"); 
  display.println("---------------------"); 
  display.print("Open Dashboard at:\n"); 
  display.println(WiFi.localIP()); 
  display.display(); 

  server.on("/", handleRootPage); 
  server.begin(); 

  pinMode(ROTARY_CLK, INPUT_PULLUP); 
  pinMode(ROTARY_DT, INPUT_PULLUP); 
  pinMode(ROTARY_SW, INPUT_PULLUP); 

  attachInterrupt(digitalPinToInterrupt(ROTARY_CLK), handleEncoderRotation, CHANGE); 
  attachInterrupt(digitalPinToInterrupt(ROTARY_SW), handleButtonPress, FALLING); 
  Serial.println("\nSystem Hybrid Ready!"); 
} 

void loop() { 
  if (WiFi.status() == WL_CONNECTED) { 
    server.handleClient(); 
  } 

  // --- Capped GPS Serial Read to prevent data choking ---
  int bytesRead = 0; 
  while (Serial2.available() > 0 && bytesRead < 150) { 
    char rawByte = Serial2.read(); 
    gps.encode(rawByte); 
    bytesRead++; 
  } 

  // --- High-Response Location State Tracking via isValid() ---
  if (gps.location.isValid()) { 
    currentLat = gps.location.lat(); 
    currentLng = gps.location.lng(); 
    currentSpeed = gps.speed.kmph(); 
    
    if (gps.altitude.isValid()) { 
      currentAltitude = gps.altitude.meters();
}
if (isTrackingActive && startLat != 0.0 && startLng != 0.0) {
distanceMeters = TinyGPSPlus::distanceBetween(startLat, startLng, currentLat, currentLng);
}
// --- EVALUATE SPEED LIMIT & DISPATCH ALERT ---
if (currentSpeed > speedLimitSetting) {
if (millis() - lastNotificationTime > alertCooldown) {
sendSpeedAlert(currentSpeed, speedLimitSetting, currentLat, currentLng);
lastNotificationTime = millis();
}
}
}
// Debounced execution logic for hardware input profiles
if (encoderChanged) {
encoderChanged = false;
Serial.print("Target Speed Threshold Adjusted: ");
Serial.print(speedLimitSetting);
Serial.println(" km/h");
}
if (buttonPressed) {
buttonPressed = false;
isTrackingActive = !isTrackingActive; // Toggle Odometer Tracking State
if (isTrackingActive) {
startLat = currentLat;
startLng = currentLng;
distanceMeters = 0.0;
}
Serial.println(isTrackingActive ? "Tracking: RUNNING" : "Tracking: STANDBY");
}
// --- Premium Graphic Layout Rendering on Physical OLED Display ---
if (millis() - lastOLEDRefresh > 300) {
lastOLEDRefresh = millis();
display.clearDisplay();
// Top Status Bar: Tracking state and Satellites lock icon info
display.setTextSize(1);
display.setTextColor(SSD1306_WHITE);
display.setCursor(0, 0);
display.print(isTrackingActive ? "RUNNING" : "STANDBY");
display.setCursor(80, 0);
display.print("SAT: ");
display.print(gps.satellites.value());
// Horizontal Border Divider Line
display.drawFastHLine(0, 10, 128, SSD1306_WHITE);
// Big Centered Main Telemetry Values Display Layout Split Screen
display.setCursor(0, 16);
display.setTextSize(2);
display.print((int)currentSpeed);
display.setTextSize(1);
display.print(" km/h");
// Distance Metric right aligned layout adjustments
display.setCursor(70, 16);
display.print("Odo (m):");
display.setCursor(70, 26);
display.print((int)distanceMeters);
// Middle section horizontal layout divider line rules
display.drawFastHLine(0, 38, 128, SSD1306_WHITE);
// Bottom Section: Speed limit configuration profile context
display.setCursor(0, 43);
display.print("LIMIT: ");
display.print(speedLimitSetting);
display.print(" km/h");
// Graphical Speed Bar indicator structure box logic calculation
display.drawRect(0, 54, 128, 7, SSD1306_WHITE);
int maxBarWidth = 124;
int fillWidth = 0;
if(speedLimitSetting > 0) {
fillWidth = ((int)currentSpeed * maxBarWidth) / speedLimitSetting;
if(fillWidth > maxBarWidth) fillWidth = maxBarWidth;
if(fillWidth < 0) fillWidth = 0;
}
display.fillRect(2, 56, fillWidth, 3, SSD1306_WHITE);
// Handle screen inversion on active overspeed breaches
if (currentSpeed > speedLimitSetting) {
display.invertDisplay(true);
} else {
display.invertDisplay(false);
}
display.display();
}
}
