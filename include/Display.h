#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include "Config.h"
#include "Sensors.h"

class Display {
private:
    Adafruit_SSD1306 oled;

public:
    Display() : oled(128, 64, &Wire, -1) {
    }

    void initialize() {
        Wire.begin();
        oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS);
    }

    void showSimpleMessage(String line1, String line2 = "") {
        oled.clearDisplay();
        oled.setTextSize(1);
        oled.setTextColor(SSD1306_WHITE);
        oled.setCursor(0, 0);
        oled.print(line1);
        oled.setCursor(0, 16);
        oled.print(line2);
        oled.display();
    }

    void showDashboard(Sensors &sensors, bool isAlert, bool isWifiOk) {
        oled.clearDisplay();
        oled.setTextSize(1);
        oled.setTextColor(SSD1306_WHITE);

        oled.setCursor(0, 0);
        if (isWifiOk) oled.printf("IP:%s", WiFi.localIP().toString().c_str());
        else oled.print("Local Mode");

        oled.setCursor(0, 14);
        oled.printf("Temp: %.1fC", sensors.temperature);
        oled.setCursor(0, 28);
        oled.printf("Gas : %d", sensors.gasLevel);
        oled.setCursor(0, 42);
        oled.printf("PIR : %s", sensors.motionDetected ? "MVMT" : "---");

        oled.setCursor(0, 56);
        if (isAlert) oled.print(">>> ALERT <<<");
        else oled.print("OK");

        oled.display();
    }
};