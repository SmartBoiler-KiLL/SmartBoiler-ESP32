#ifndef MEMORY_H
#define MEMORY_H

#include <EEPROM.h>

class Memory {
public:
    static constexpr uint8_t EEPROM_SIZE = 250;
    static constexpr uint8_t SSID_ADDRESS = 0;
    static constexpr uint8_t PASS_ADDRESS = 75;
    static constexpr uint8_t APP_ID_ADDRESS = 150;
    static constexpr uint8_t TEMPERATURE_ADDRESS = 200;

    static void initialize();
    /// @brief Verifies if the memory content is valid (SSID, password, app id)
    static bool verifyContent();

    static String getSSID();
    static String getPassword();
    static String getAppId();
    static int getTemperature();

    static void write(String ssid, String password, String appId);
    static void writeTemperature(int temperature);

    /// @brief Clears the EEPROM, erasing all the content.
    static void clear();
};

#endif
