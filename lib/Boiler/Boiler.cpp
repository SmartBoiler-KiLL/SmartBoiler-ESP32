#include <Arduino.h>
#include "Boiler.h"
#include "Memory.h"
#include "TemperatureSensor.h"
#include "KiLL.h"

Boiler* Boiler::instance = nullptr;

Boiler::Boiler() {
    temperatureSensor = new TemperatureSensor();
    currentTemperature = 0;
    isOn = false;
    lastMinTempUpdate = 0;
    shouldUpdateMinTemp = true;
    minimumTemperature = 0;
    pid = nullptr;
}

void IRAM_ATTR Boiler::zeroCrossISR() {
    if (!instance) return;

    // Enciende si estamos dentro de los ciclos deseados
    if (instance->zeroCrossCounter < instance->desiredActiveCycles) {
        digitalWrite(SSR_ACTIVAION_PIN, HIGH);
    } else {
        digitalWrite(SSR_ACTIVAION_PIN, LOW);
    }

    // Incrementar y reiniciar el ciclo cada 10 cruces
    instance->zeroCrossCounter++;
    if (instance->zeroCrossCounter >= Boiler::totalCycles) {
        instance->zeroCrossCounter = 0;
    }
}

void Boiler::setPowerPercent(uint8_t percent) {
    if (percent > 100) percent = 100;
    desiredActiveCycles = (percent * totalCycles) / 100;  
}

void Boiler::begin() {
    pinMode(SSR_ACTIVAION_PIN, OUTPUT);
    pinMode(BOILER_LED_PIN, OUTPUT);
    temperatureSensor->begin();
    targetTemperature = Memory::getTemperature();
    currentTemperature = temperatureSensor->readTemperature(0);
    
    lastMinTempUpdate = millis();
    shouldUpdateMinTemp = true;
    
    instance = this;
    attachInterrupt(digitalPinToInterrupt(ZERO_CROSS_DETECTION_PIN), Boiler::zeroCrossISR, RISING);

    pid = new PID();
}


void Boiler::setTargetTemperature(int temperature) {
    targetTemperature = temperature;
}

void Boiler::turnOn() {
    targetTemperature = constrain(targetTemperature, minimumTemperature, KiLL::MAXIMUM_TEMPERATURE);
    Serial.println("[Boiler] Turning on");
    isOn = true;
    shouldUpdateMinTemp = false;
    digitalWrite(BOILER_LED_PIN, HIGH);
}

void Boiler::turnOff() {
    Memory::writeTemperature(targetTemperature);
    Serial.println("[Boiler] Turning off");
    isOn = false;
    shouldUpdateMinTemp = true;
    setPowerPercent(0);
    digitalWrite(BOILER_LED_PIN, LOW);
}

void Boiler::toggle() {
    if (isOn) {
        turnOff();
    } else {
        turnOn();
    }
}

double Boiler::getCurrentTemperature() {
    return currentTemperature;
}

bool Boiler::getIsOn() {
    return isOn;
}

int Boiler::getTargetTemperature() {
    return targetTemperature;
}

int Boiler::getMinimumTemperature() {
    return minimumTemperature;
}

void Boiler::updateMinimumTemperature() {
    if (!shouldUpdateMinTemp) return;
    
    unsigned long currentTime = millis();
    
    // Update minimum temperature if:
    // 1. It hasn't been set yet (minimumTemperature == 0)
    // 2. MIN_TEMP_UPDATE_INTERVAL has elapsed since last update
    bool shouldUpdate = (minimumTemperature == 0) || 
                  (lastMinTempUpdate > 0 && currentTime - lastMinTempUpdate >= MIN_TEMP_UPDATE_INTERVAL);
    
    if (shouldUpdate) {
        lastMinTempUpdate = currentTime;
        minimumTemperature = round(currentTemperature + 2);
        Serial.println("[Boiler] Minimum temperature updated: " + String(minimumTemperature) + "°C");
    }
}

double Boiler::controlTemperature() {
    currentTemperature = temperatureSensor->readTemperature(0);
    updateMinimumTemperature();

    if (!isOn) return currentTemperature;
    
    int power = pid->control(targetTemperature, currentTemperature, minimumTemperature, KiLL::MAXIMUM_TEMPERATURE);
    setPowerPercent(power);
    Serial.println("[Boiler] Power: " + String(power) + "%");

    return currentTemperature;
}
