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
    bool motionDetected = false;

    Sensors() : dht(PIN_DHT22, DHT22) {
    }

    void initialize() {
        pinMode(PIN_PIR, INPUT);
        dht.begin();
    }

    void readAll() {
        readClimate();
        readMotion();
    }

    // DHT22: at most one read every 2 s
    void readClimate() {
        temperature = dht.readTemperature();
        humidity = dht.readHumidity();
    }

    // PIR:  read before each telemetry message
    void readMotion() {
        gasLevel = analogRead(PIN_MQ2_AO);

        if (millis() > PIR_WARMUP_MS) {
            motionDetected = (digitalRead(PIN_PIR) == HIGH);
        }
    }
};
