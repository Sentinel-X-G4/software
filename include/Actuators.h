#pragma once
#include <Arduino.h>
#include "Config.h"

class Actuators {
public:
    bool isAlertActive = false;

    void initialize() {
        pinMode(PIN_BUZZER, OUTPUT);
        pinMode(PIN_LED_R, OUTPUT);
        pinMode(PIN_LED_V, OUTPUT);
        turnOffAll();
    }

    void turnOffAll() {
        digitalWrite(PIN_BUZZER, LOW);
        digitalWrite(PIN_LED_R, LOW);
        digitalWrite(PIN_LED_V, LOW);
    }

    void triggerAlert(bool active) {
        isAlertActive = active;
        if (active) {
            digitalWrite(PIN_BUZZER, HIGH);
            digitalWrite(PIN_LED_R, HIGH);
            digitalWrite(PIN_LED_V, LOW);
        } else {
            digitalWrite(PIN_BUZZER, LOW);
            digitalWrite(PIN_LED_R, LOW);
            digitalWrite(PIN_LED_V, HIGH);
        }
    }
};