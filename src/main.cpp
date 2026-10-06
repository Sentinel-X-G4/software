#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <ArduinoJson.h>

constexpr uint8_t PIN_SDA = D2; // GPIO4
constexpr uint8_t PIN_SCL = D1; // GPIO5
constexpr uint8_t PIN_DHT = D5; // GPIO14
constexpr uint8_t PIN_PIR = D6; // GPIO12
constexpr uint8_t PIN_MQ2_DO = D7; // GPIO13 (optionnel, via pont diviseur)
constexpr uint8_t PIN_MQ2_AO = A0; // ADC0 (via pont diviseur 10k/20k)

constexpr uint32_t READ_INTERVAL_MS = 2000; // DHT22 : 1 lecture max / 2 s
constexpr uint32_t PIR_WARMUP_MS = 60000; // le PIR chauffe 30-60 s
constexpr uint32_t MQ2_WARMUP_MS = 180000; // quelques minutes (24 h au 1er usage)

constexpr uint8_t OLED_ADDR = 0x3C;
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
DHT dht(PIN_DHT, DHT22);

struct Measures {
    float temperature = NAN; // °C
    float humidity = NAN; // % HR
    int gasRaw = 0; // 0..1023 (valeur relative, pas des ppm)
    bool gasAlarm = false; // DO : LOW = seuil dépassé
    bool motion = false;
};

Measures m;
uint32_t lastRead = 0;

void readSensors() {
    m.temperature = dht.readTemperature();
    m.humidity = dht.readHumidity();
    m.gasRaw = analogRead(PIN_MQ2_AO);
    m.gasAlarm = (digitalRead(PIN_MQ2_DO) == LOW);
    m.motion = (digitalRead(PIN_PIR) == HIGH);
}

void drawDisplay() {
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);

    oled.setCursor(0, 0);
    if (isnan(m.temperature)) oled.print("Temp : --");
    else oled.printf("Temp : %.1f C", m.temperature);

    oled.setCursor(0, 14);
    if (isnan(m.humidity)) oled.print("Hum  : --");
    else oled.printf("Hum  : %.1f %%", m.humidity);

    oled.setCursor(0, 28);
    oled.printf("Gaz  : %d%s", m.gasRaw, m.gasAlarm ? " !" : "");

    oled.setCursor(0, 42);
    oled.printf("PIR  : %s", m.motion ? "MOUVEMENT" : "rien");

    if (millis() < MQ2_WARMUP_MS) {
        oled.setCursor(0, 56);
        oled.print("(chauffe capteurs)");
    }
    oled.display();
}

void sendPayload() {
    JsonDocument doc;
    doc["uptime_s"] = millis() / 1000;

    if (isnan(m.temperature)) doc["temperature"] = nullptr;
    else doc["temperature"] = round(m.temperature * 10) / 10.0;
    if (isnan(m.humidity)) doc["humidity"] = nullptr;
    else doc["humidity"] = round(m.humidity * 10) / 10.0;

    doc["gas_raw"] = m.gasRaw;
    doc["gas_alarm"] = m.gasAlarm;
    doc["motion"] = m.motion;
    doc["warmup"] = millis() < MQ2_WARMUP_MS;

    serializeJson(doc, Serial);
    Serial.println();
}

void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n[boot] Phase 1");

    pinMode(PIN_PIR, INPUT);
    pinMode(PIN_MQ2_DO, INPUT);

    Wire.begin(PIN_SDA, PIN_SCL);
    if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
        Serial.println("[erreur] OLED introuvable (adresse 0x3C/0x3D ? cablage ?)");
    }
    dht.begin();

    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);
    oled.print("Demarrage...");
    oled.display();
}

void loop() {
    // millis() plutôt que delay() : le programme ne se bloque jamais
    if (millis() - lastRead >= READ_INTERVAL_MS) {
        lastRead = millis();
        readSensors();
        drawDisplay();
        sendPayload();
    }
}
