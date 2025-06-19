#include "KiLL.h"

#include <Arduino.h>    

#include "Memory.h"        
#include "GlobalNetwork.h"
#include "LocalNetwork.h" 
#include "Boiler.h"
#include "Display.h"

KiLL::KiLL() {
    display = new Display();
    boiler = new Boiler();
    
    localNetwork = new LocalNetwork(*boiler, *display);      
    globalNetwork = new GlobalNetwork(*localNetwork, *boiler);
    factoryButtonPressed = false;
    factoryButtonPressedTime = 0;
}

KiLL::~KiLL() {
    delete localNetwork;
    delete globalNetwork;
    delete boiler;
    delete display;
    localNetwork = nullptr;
    globalNetwork = nullptr;
    boiler = nullptr;
    display = nullptr;
}

void KiLL::setup() {
    pinMode(FACTORY_RESET_PIN, INPUT_PULLUP);
    pinMode(PIN_INCREASE_TARGET_TEMPERATURE, INPUT_PULLUP);
    pinMode(PIN_DECREASE_TARGET_TEMPERATURE, INPUT_PULLUP);
    pinMode(PIN_TOGGLE_BOILER, INPUT_PULLUP);

    Memory::initialize();
    
    localNetwork->initialize();
    localNetwork->setupLocalNetwork();
    localNetwork->setupServer();

    if (Memory::verifyContent()) {
        globalNetwork->startWiFiConnection();
    }

    boiler->begin();
    display->beginDisplay(boiler->getTargetTemperature(), boiler->getCurrentTemperature());
}

const String KiLL::espId() {
    uint64_t chipId = ESP.getEfuseMac();
    char buffer[13];
    sprintf(buffer, "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
    return String(buffer);
}

void KiLL::keepServersAlive() {
    // if (globalNetwork->isConnectedToWifi()) {
    //     globalNetwork->keepConnectionWithServer();
    // }
}

void KiLL::tryToReconnectToWifi() {
    globalNetwork->tryReconnectWiFi();
}

void KiLL::checkForFactoryReset() {
    bool isButtonNowPressed = digitalRead(FACTORY_RESET_PIN) == LOW;

    // Button pressed for the first time
    if (isButtonNowPressed && !factoryButtonPressed) {
        factoryButtonPressed = true;
        factoryButtonPressedTime = millis();
    } 
    
    // Button pressed for FACTORY_RESET_HOLD_TIME, reset to factory settings
    if (isButtonNowPressed && millis() - factoryButtonPressedTime >= FACTORY_RESET_HOLD_TIME) {
        Serial.println("[KiLL] Factory reset button pressed for too long. Resetting to factory settings...");
        resetToFactorySettings();
    }

    // Button released before timeout
    if (!isButtonNowPressed && factoryButtonPressed) {
        factoryButtonPressed = false;
        factoryButtonPressedTime = 0;
    }    
}

void KiLL::resetToFactorySettings() {
    Serial.println("[KiLL] Resetting to factory settings...");
    Memory::clear();
    delay(3000);
    ESP.restart();
}

void KiLL::checkUserInteraction() {
    checkForFactoryReset();

    if (digitalRead(PIN_TOGGLE_BOILER) == LOW) {
        boiler->toggle();
        display->updateBoilerStatus(boiler->getIsOn());
        delay(200);
    }
    
    bool isBoilerOn = boiler->getIsOn();
    display->updateBoilerStatus(isBoilerOn);
    if (!isBoilerOn) return;

    int currentTargetTemperature = boiler->getTargetTemperature();

    if (digitalRead(PIN_DECREASE_TARGET_TEMPERATURE) == LOW) {
        if (currentTargetTemperature > boiler->getMinimumTemperature()) {
            currentTargetTemperature--;
            boiler->setTargetTemperature(currentTargetTemperature);
            display->updateTargetTemperature(currentTargetTemperature);
        }
        delay(200);
    }

    if (digitalRead(PIN_INCREASE_TARGET_TEMPERATURE) == LOW) {
        if (currentTargetTemperature < MAXIMUM_TEMPERATURE) {
            currentTargetTemperature++;
            boiler->setTargetTemperature(currentTargetTemperature);
            display->updateTargetTemperature(currentTargetTemperature);
        }
        delay(200);
    }
}

void KiLL::controlTemperature(){
    display->updateCurrentTemperature(boiler->controlTemperature());
}