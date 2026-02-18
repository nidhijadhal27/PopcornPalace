#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>
#include <LiquidCrystal_I2C.h>

#define DHTPIN 4
#define DHTTYPE DHT22
#define GAS_PIN 33
#define PIR_PIN 27
#define RELAY_PIN 25
#define BUZZER_PIN 26

DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* apiKey = "MJ0N4UDMMA0NBQRB";
const char* server = "http://api.thingspeak.com/update";

unsigned long lastUpdate = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();
  lcd.init();
  lcd.backlight();

  pinMode(GAS_PIN, INPUT);
  pinMode(PIR_PIN, INPUT);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  WiFi.begin(ssid, password);
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("Connected!");
  lcd.clear();
  lcd.print("Greenhouse Ready");
  delay(1000);
}

void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  int gasRaw = analogRead(GAS_PIN);
  float gasPct = gasRaw * 100.0 / 4095.0;
  int pir = digitalRead(PIR_PIN);

  // LCD display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.printf("T:%.1f H:%.1f", t, h);
  lcd.setCursor(0, 1);
  lcd.printf("G:%.0f%% PIR:%d", gasPct, pir);

  // Relay logic
  if (t > 50) digitalWrite(RELAY_PIN, HIGH);
  else digitalWrite(RELAY_PIN, LOW);

  // PIR + buzzer
  if (pir == 1) {
    digitalWrite(BUZZER_PIN, HIGH);
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

  // Send to ThingSpeak every 16 sec
  if (millis() - lastUpdate > 16000) {
    sendToThingSpeak(t, h, gasPct, pir);
    lastUpdate = millis();
  }

  delay(1000);
}

void sendToThingSpeak(float t, float h, float gas, int pir) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = String(server) + "?api_key=" + apiKey +
                 "&field1=" + String(t) +
                 "&field2=" + String(h) +
                 "&field3=" + String(gas) +
                 "&field4=" + String(pir);
    http.begin(url);
    int code = http.GET();
    if (code > 0) {
      Serial.println("Sent to ThingSpeak");
    } else {
      Serial.println("ThingSpeak error");
    }
    http.end();
  }
}
