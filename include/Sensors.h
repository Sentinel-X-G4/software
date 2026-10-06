#pragma once
#include <Arduino.h>
#include <DHT.h>
#include "Config.h"

class Sensors {
private:
    DHT dht;

public:
    float temperature = NAN;
    float humidity = NAN;
    int gasLevel = 0;
    bool gasAlarm = false;
    bool motionDetected = false;

    Sensors() : dht(PIN_DHT22, DHT22) {
    }

    void initialize() {
        pinMode(PIN_PIR, INPUT);
        pinMode(PIN_MQ2_DO, INPUT);
        dht.begin();
    }

    void readAll() {
        temperature = dht.readTemperature();
        humidity = dht.readHumidity();
        gasLevel = analogRead(PIN_MQ2_AO);
        gasAlarm = (digitalRead(PIN_MQ2_DO) == LOW);

        if (millis() > PIR_WARMUP_MS) {
            motionDetected = (digitalRead(PIN_PIR) == HIGH);
        }
    }

    bool checkCriticalThresholds() {
        return gasAlarm || (gasLevel > THRESHOLD_GAS_MAX) ||
               (!isnan(temperature) && temperature > THRESHOLD_TEMP_MAX);
    }
};
