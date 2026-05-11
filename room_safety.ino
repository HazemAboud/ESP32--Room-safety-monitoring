#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define DHTPIN 4
#define DHTTYPE DHT11
#define MQ2_PIN 32
#define MIC_PIN 33

DHT dht(DHTPIN, DHTTYPE);
WebServer server(80);

const char* ssid = "Hazem-ESP32-Monitor";
const char* password = "12345678";

unsigned long displayStartTime = 0;
const unsigned long displayDuration = 20000;
bool isDisplayActive = false;
bool forceAlwaysOn = false;

float h, t;
int rawGas;
String airText, soundText, temp_v, roomStatus;

void updateTempStatus(float currentTemp) {
  if (currentTemp >= 20.0 && currentTemp <= 26.0) temp_v = "Optimal";
  else if (currentTemp > 26.0 && currentTemp <= 30.0) temp_v = "Normal";
  else if ((currentTemp >= 10.0 && currentTemp < 20.0) || (currentTemp > 30.0 && currentTemp <= 38.0)) temp_v = "Caution";
  else temp_v = "Danger";
}

String getAirQuality(float raw) {
  if (raw <= 600) return "Normal";
  if (raw <= 1200) return "Caution";
  return "Danger";
}

String getSoundStatus(int ptp) {
  if (ptp < 30) return "Quiet";
  if (ptp < 70) return "Normal";
  if (ptp < 150) return "Caution";
  return "Danger";
}

void calculateRoomStatus() {
  if (temp_v == "Danger" || airText == "Danger" || soundText == "Danger") {
    roomStatus = "Danger";
    return;
  }

  int c_counter = 0;
  if (temp_v == "Caution") c_counter++;
  if (airText == "Caution") c_counter++;
  if (soundText == "Caution") c_counter++;

  if (c_counter >= 2) {
    roomStatus = "Danger";   
  } else if (c_counter == 1) {
    roomStatus = "Caution";
  } else {
    roomStatus = "Normal";
  }
}

void handleRoot() {
  String html = "<html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='1'>";
  html += "<style>body{font-family:'Segoe UI',sans-serif; text-align:center; background:#eceff1; color:#37474f; margin:0; padding:20px;} ";
  html += ".card{background:white; padding:25px; margin:15px auto; max-width:450px; border-radius:12px; box-shadow:0 8px 16px rgba(0,0,0,0.08);} ";
  html += ".btn{padding:14px; margin:5px; width:45%; font-size:13px; font-weight:bold; border-radius:6px; border:none; color:white; cursor:pointer; text-decoration:none; display:inline-block;} ";
  html += ".on{background:#2e7d32;} .off{background:#455a64;} ";
  html += ".status-box{font-size:32px; font-weight:bold; padding:15px; margin:15px 0; border-radius:8px; border:2px solid; ";

  if (roomStatus == "Normal") html += "background:#006400; color:#2e7d32; border-color:#a5d6a7;} ";
  else if (roomStatus == "Caution") html += "background:#FFA500; color:#f57f17; border-color:#fff59d;} ";
  else if (roomStatus == "Danger") html += "background:#FF0000; color:#c62828; border-color:#ef9a9a;} ";
  else html += "background:#b71c1c; color:white; border-color:#000;} ";

  html += "</style></head><body>";
  html += "<div class='card'><h2>Room Status</h2>";
  html += "<div class='status-box'>" + roomStatus + "</div></div>";
  html += "<div class='card'><h3>Core Metrics</h3>";
  html += "<p>Temperature: <b>" + String(t, 1) + " C</b></p>";
  html += "<p>Humidity: <b>" + String(h, 1) + " %</b></p>";
  html += "<p>Air Quality: <b>" + airText + "</b></p>";
  html += "<p>Acoustic Level: <b>" + soundText + "</b></p></div>";
  html += "<div class='card'><p>System Mode: <b>" + String(forceAlwaysOn ? "On" : "Auto") + "</b></p>";
  html += "<a href='/on' class='btn on'>Force Active</a><a href='/sleep' class='btn off'>Enable Auto</a></div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.clearDisplay();
  display.display();
  dht.begin();
  analogReadResolution(12);
  WiFi.softAP(ssid, password);
  server.on("/", handleRoot);
  server.on("/on", []() {
    forceAlwaysOn = true;
    isDisplayActive = true;
    server.sendHeader("Location", "/");
    server.send(303);
  });
  server.on("/sleep", []() {
    forceAlwaysOn = false;
    isDisplayActive = false;
    displayStartTime = 0;
    server.sendHeader("Location", "/");
    server.send(303);
  });
  server.begin();
}

void loop() {
  server.handleClient();
  h = dht.readHumidity();
  t = dht.readTemperature();
  rawGas = analogRead(MQ2_PIN);
  updateTempStatus(t);
  airText = getAirQuality(rawGas);

  unsigned int sMax = 0, sMin = 4095;
  unsigned long start = millis();
  while (millis() - start < 150) {
    int s = analogRead(MIC_PIN);
    if (s > sMax) sMax = s;
    if (s < sMin) sMin = s;
  }
  int ptp = sMax - sMin;
  soundText = getSoundStatus(ptp);
  calculateRoomStatus();

  if (forceAlwaysOn) isDisplayActive = true;
  else {
    if (ptp >= 100) {
      isDisplayActive = true;
      displayStartTime = millis();
    }
    if (isDisplayActive && (millis() - displayStartTime > displayDuration)) {
      isDisplayActive = false;
    }
  }

  display.clearDisplay();
  if (isDisplayActive) {
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("Room Status:");
    display.setCursor(0, 12);
    display.println(roomStatus);
    display.drawFastHLine(0, 28, 128, SSD1306_WHITE);
    display.setCursor(0, 32);
    display.printf("T:%.1fC  H:%.0f%%", t, h);
    display.setCursor(0, 44);
    display.print("Air:   ");
    display.println(airText);
    display.setCursor(0, 54);
    display.print("Sound: ");
    display.println(soundText);
  }
  display.display();
}