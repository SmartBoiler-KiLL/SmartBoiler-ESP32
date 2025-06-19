#ifndef DISPLAY_OLED_H
#define DISPLAY_OLED_H

#include <Arduino.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Wire.h>

class Display {
public:
    Display();
    void beginDisplay(int initialTargetTemperature, double currentTemperature);
    void updateTargetTemperature(int newTargetTemperature);
    void updateCurrentTemperature(double newCurrentTemperature);
    void updateBoilerStatus(bool isBoilerOn);

private:
    Adafruit_SSD1306* display;
    static constexpr uint16_t SCREEN_WIDTH = 128;
    static constexpr uint16_t SCREEN_HEIGHT = 64;

    int targetTemperature;
    double currentTemperature;
    bool boilerOn;

    /// @brief Update all the display with the current set point and true value.
    void updateAll(); 
};

#endif
