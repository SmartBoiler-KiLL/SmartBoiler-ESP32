#include "TemperatureSensor.h"

TemperatureSensor::TemperatureSensor() {
    ads = new Adafruit_ADS1115();
    windowIndex = 0;
    windowFilled = false;    
    memset(temperatureWindow, 0, sizeof(temperatureWindow));
}

void TemperatureSensor::begin() {
    ads->begin();
    ads->setGain(GAIN_SIXTEEN); 
}

double TemperatureSensor::getVCC() {
    int16_t rawADC = ads->readADC_SingleEnded(2);
    return (rawADC * 0.256) / 32767.0;
}

double TemperatureSensor::readTemperature(uint8_t channel) {
    // TODO: Remove the comments when using the real sensor
    // int16_t rawADC = ads->readADC_SingleEnded(channel);
    
    // double vOut = (rawADC * 0.256) / 32767.0;
    // double R_PT100 = (-vOut * R_FIXED) / (vOut - 3.3);
    // double rawTemperature = (R_PT100 - R0) / (alpha * R0);
    
    // return getFilteredTemperature(abs(rawTemperature));
    return random(23, 66);
}

double TemperatureSensor::getFilteredTemperature(double newReading) {
    temperatureWindow[windowIndex] = newReading;
    windowIndex = (windowIndex + 1) % TEMPERATURE_WINDOW_SIZE;
    
    if (windowIndex == 0) windowFilled = true;
    
    double sum = 0.0;
    uint8_t count = windowFilled ? TEMPERATURE_WINDOW_SIZE : windowIndex;
    
    for (uint8_t i = 0; i < count; i++) sum += temperatureWindow[i];
    return sum / count;
}
