#include "Display.h"

Display::Display() {
    display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
    targetTemperature = 0;
    currentTemperature = 0.0;
    boilerOn = false;
}

void Display::beginDisplay(int initialTargetTemperature, double currentTemperature) {
    Wire.begin(21, 22);

    if (!display->begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Fallo al iniciar la pantalla OLED"));
    }

    display->clearDisplay();
    display->display();
    targetTemperature = initialTargetTemperature;
    currentTemperature = currentTemperature;
    updateAll();
}

void Display::updateTargetTemperature(int newTargetTemperature) {
    targetTemperature = newTargetTemperature;
    updateAll();
}

void Display::updateCurrentTemperature(double newCurrentTemperature) {
    currentTemperature = newCurrentTemperature;
    updateAll();
}

void Display::updateBoilerStatus(bool isBoilerOn) {
    boilerOn = isBoilerOn;
    updateAll();
}

void Display::updateAll() {
    display->clearDisplay();

    display->setTextSize(2);
    display->setTextColor(SSD1306_WHITE);
    display->setCursor(0, 0);
    
    if (boilerOn) {
        display->print(targetTemperature);
        int x = display->getCursorX();
        display->drawCircle(x + 2, 5, 2, SSD1306_WHITE);
        display->setCursor(x + 8, 0);
        display->print("C");
    } else {
        display->print("OFF");
    }

    display->setTextSize(1);
    display->setCursor(5, 40);
    display->print(currentTemperature, 2);
    int x = display->getCursorX();
    display->drawCircle(x + 2, 42, 1, SSD1306_WHITE);
    display->setCursor(x + 8, 40);
    display->print("C");

    display->display();
}
