#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include "Config.h"
#include "Sensors.h"

// Auto: sensor dashboard; Off / Message are forced by a remote command and persist until "reset" or a reboot
enum class ScreenMode : uint8_t { Auto, Off, Message };
constexpr size_t SCREEN_MESSAGE_MAX = 100; // 21 characters x 5 lines at text size 1

class Display {
private:
    Adafruit_SSD1306 oled;

    void setPower(bool on) {
        oled.ssd1306_command(on ? SSD1306_DISPLAYON : SSD1306_DISPLAYOFF);
    }

public:
    ScreenMode mode = ScreenMode::Auto;

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

    // Back to the dashboard (refreshed by the next showDashboard())
    void setAuto() {
        mode = ScreenMode::Auto;
        setPower(true);
    }

    void turnOff() {
        mode = ScreenMode::Off;
        setPower(false);
    }

    // Default font: ASCII only, control characters are replaced with spaces
    void showMessage(const char *text) {
        mode = ScreenMode::Message;
        setPower(true);
        oled.clearDisplay();
        oled.setTextSize(1);
        oled.setTextColor(SSD1306_WHITE);
        oled.setTextWrap(true);
        oled.setCursor(0, 0);
        for (size_t i = 0; text[i] != '\0' && i < SCREEN_MESSAGE_MAX; i++) {
            oled.write(text[i] >= 0x20 && text[i] < 0x7f ? text[i] : ' ');
        }
        oled.display();
    }

    void showDashboard(Sensors &sensors, bool isAlert, bool isWifiOk) {
        if (mode != ScreenMode::Auto) return;
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