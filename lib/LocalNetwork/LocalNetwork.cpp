#include "LocalNetwork.h"

#include "Memory.h"
#include "KiLL.h"
#include "Display.h"

LocalNetwork::LocalNetwork(Boiler& boiler, Display& display) : webSocketServer(WEBSOCKET_PORT), boiler(boiler), display(display) {}

const String LocalNetwork::getHostname() {
    return "ws://KiLL-" + KiLL::espId() + ".local:" + String(WEBSOCKET_PORT) + "/";
}

const String LocalNetwork::SSID() {
    return "KiLL-" + KiLL::espId();
}

const String LocalNetwork::STATUS_RESPONSE(const String message) {
    return "{\"status\": \"" + message + "\"}";
}

const String LocalNetwork::OK_RESPONSE = STATUS_RESPONSE("OK");

const String LocalNetwork::ERROR_RESPONSE(const String message) {
    return "{\"error\": \"" + message + "\"}";
}

const String LocalNetwork::MISSING_AUTHENTICATION_RESPONSE = ERROR_RESPONSE("Missing authentication");

void LocalNetwork::onStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.print("[LocalNetwork] Station connected: ");
    for (uint8_t i = 0; i < 6; i++) {
        Serial.printf("%02X", info.wifi_ap_staconnected.mac[i]);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
}

void LocalNetwork::onStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.print("[LocalNetwork] Station disconnected: ");
    for (uint8_t i = 0; i < 6; i++) {
        Serial.printf("%02X", info.wifi_ap_stadisconnected.mac[i]);
        if (i < 5) Serial.print(":");
    }
    Serial.println();
}

void LocalNetwork::initialize() {
    WiFi.mode(WIFI_AP_STA);
    
    // Configure custom IP for soft AP
    IPAddress local_IP(192, 168, 39, 12);
    IPAddress gateway(192, 168, 39, 1);
    IPAddress subnet(255, 255, 255, 0);
    
    WiFi.softAPConfig(local_IP, gateway, subnet);
    WiFi.softAP(SSID());
    
    Serial.println("[LocalNetwork] WiFi Access Point started");
    Serial.println("[LocalNetwork] IP Address: " + WiFi.softAPIP().toString());

    // Register Wi-Fi event handlers
    WiFi.onEvent(onStationConnected, ARDUINO_EVENT_WIFI_AP_STACONNECTED);
    WiFi.onEvent(onStationDisconnected, ARDUINO_EVENT_WIFI_AP_STADISCONNECTED);
}

void LocalNetwork::stopAccessPoint() {
    WiFi.softAPdisconnect(true);
    Serial.println("[LocalNetwork] WiFi Access Point stopped");
}

void LocalNetwork::setupServer() {
    webSocketServer.onEvent(std::bind(&LocalNetwork::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    startServer();
}

void LocalNetwork::startServer() {
    webSocketServer.begin();
    Serial.println("[LocalNetwork] WebSocket server in dual core started at " + getHostname());
    
    // Create task on Core 1
    xTaskCreatePinnedToCore(
        webSocketTask,          // Task function
        "WebSocketTask",        // Task name
        4096,                   // Stack size
        this,                   // Parameter passed to task
        1,                      // Priority
        &webSocketTaskHandle,   // Task handle
        1                       // Core ID (changed from 0 to 1)
    );
}

void LocalNetwork::stopServer() {
    if (webSocketTaskHandle != NULL) {
        vTaskDelete(webSocketTaskHandle);
        webSocketTaskHandle = NULL;
    }
    webSocketServer.close();
    Serial.println("[LocalNetwork] WebSocket server stopped");
}

void LocalNetwork::webSocketTask(void* parameter) {
    LocalNetwork* localNetwork = static_cast<LocalNetwork*>(parameter);
    
    while (true) {
        localNetwork->webSocketServer.loop();
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void LocalNetwork::setupLocalNetwork() {
    Serial.print("[LocalNetwork] Setting up local network");
    String hostname = SSID();
    
    uint8_t retryAttempt = 0;
    while (!MDNS.begin(hostname) && retryAttempt <= MAX_MDNS_RETRIES) {
        Serial.print(".");
        delay(1000);
        retryAttempt++;
    }

    if (retryAttempt >= MAX_MDNS_RETRIES) {
        Serial.println("\n[LocalNetwork] Error setting up MDNS responder! Restarting...");
        ESP.restart();
    } else {
        Serial.println("\n[LocalNetwork] mDNS responder started");
    }
}

// MARK: WebSocket Event Handling

void LocalNetwork::webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.printf("[LocalNetwork] Client %u disconnected\n", num);
            break;
            
        case WStype_CONNECTED: {
            IPAddress ip = webSocketServer.remoteIP(num);
            Serial.printf("[LocalNetwork] Client %u connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
            break;
        }
        
        case WStype_TEXT:
            handleWebSocketMessage(num, String((char*)payload));
            break;
            
        default:
            break;
    }
}

void LocalNetwork::handleWebSocketMessage(uint8_t num, const String& message) {
    JsonDocument document;
    DeserializationError error = deserializeJson(document, message);
    
    if (error) {
        Serial.println("[LocalNetwork] Error parsing message: " + String(error.c_str()));
        sendWebSocketResponse(num, ERROR_RESPONSE("Invalid JSON"));
        return;
    }
    
    String action = document["action"] | "";
    
    if (action == "ping") {
        handleRootMessage(num);
    } else if (action == "local") {
        handleLocalMessage(num);
    } else if (action == "setup") {
        handleSetupMessage(num, document);
    } else if (action == "kill_reset_factory") {
        handleResetFactoryMessage(num, document);
    } else if (action == "command") {
        handleCommandMessage(num, document);
    } else if (action == "status") {
        handleStatusMessage(num, document);
    } else {
        sendWebSocketResponse(num, ERROR_RESPONSE("Unknown action"));
    }
}

void LocalNetwork::sendWebSocketResponse(uint8_t num, const String& response) {
    webSocketServer.sendTXT(num, response.c_str());
}

// MARK: Message Handlers

bool LocalNetwork::checkRequestData(JsonDocument& document, const String& message, const String source) {
    if (message.length() == 0) {
        Serial.println("[LocalNetwork] Error: No data on " + source);
        return false;
    }

    DeserializationError error = deserializeJson(document, message);
    if (error) {
        Serial.println("[LocalNetwork] Error: Failed to parse " + source + " data");
        return false;
    }

    return true;
}

void LocalNetwork::handleRootMessage(uint8_t num) {
    sendWebSocketResponse(num, STATUS_RESPONSE("KiLL"));
}

void LocalNetwork::handleLocalMessage(uint8_t num) {
    sendWebSocketResponse(num, STATUS_RESPONSE(KiLL::espId()));
}

void LocalNetwork::handleSetupMessage(uint8_t num, JsonDocument& document) {
    if (Memory::verifyContent()) {
        Serial.println("[LocalNetwork] Error: Tried to setup KiLL twice.");
        sendWebSocketResponse(num, ERROR_RESPONSE("KiLL already setup."));
        return;
    }

    String ssid = document["ssid"] | "";
    String password = document["password"] | "";
    String appId = document["appId"] | "";

    if (ssid.length() == 0 || password.length() == 0 || appId.length() == 0) {
        Serial.println("[LocalNetwork] Error: Missing data on setup. SSID: " + ssid + ", Password: " + password + ", App ID: " + appId);
        sendWebSocketResponse(num, ERROR_RESPONSE("Missing data"));
        return;
    }

    Serial.println("[LocalNetwork] Received data on setup: SSID: " + ssid + ", Password: " + password + ", App ID: " + appId);
    
    sendWebSocketResponse(num, OK_RESPONSE);
    Memory::write(ssid, password, appId);
}

void LocalNetwork::handleResetFactoryMessage(uint8_t num, JsonDocument& document) {
    if (Utils::verifyRequest(document)) {
        sendWebSocketResponse(num, OK_RESPONSE);
        KiLL::resetToFactorySettings();
    } else {
        sendWebSocketResponse(num, MISSING_AUTHENTICATION_RESPONSE);
    }
}

void LocalNetwork::handleCommandMessage(uint8_t num, JsonDocument& document) {
    if (!Utils::verifyRequest(document)) {
        sendWebSocketResponse(num, MISSING_AUTHENTICATION_RESPONSE);
        return;
    }

    String command = document["command"].as<String>();
    String value = document["value"].as<String>();

    if (command == "turn_on") {
        boiler.turnOn();
    } else if (command == "turn_off") {
        boiler.turnOff();
    } else if (command == "set_temperature") {
        Serial.println("[LocalNetwork] Setting temperature to " + value);
        
        int temperature = value.toInt();
        
        if (temperature < boiler.getMinimumTemperature() || temperature > KiLL::MAXIMUM_TEMPERATURE) {
            Serial.println("Error temperature " + String(temperature));
            sendWebSocketResponse(num, ERROR_RESPONSE("Temperature " + String(temperature) + " out of range"));
            return;
        }

        boiler.setTargetTemperature(temperature);
        display.updateTargetTemperature(temperature);
    } else {
        sendWebSocketResponse(num, ERROR_RESPONSE("Unknown command: " + command));
        return;
    }

    sendWebSocketResponse(num, OK_RESPONSE);
}

void LocalNetwork::handleStatusMessage(uint8_t num, JsonDocument& document) {
    if (!Utils::verifyRequest(document)) {
        sendWebSocketResponse(num, MISSING_AUTHENTICATION_RESPONSE);
        return;
    }

    JsonDocument response;
    response["targetTemperature"] = boiler.getTargetTemperature();
    response["currentTemperature"] = boiler.getCurrentTemperature();
    response["isOn"] = boiler.getIsOn();
    response["localIP"] = (WiFi.status() != WL_CONNECTED ? WiFi.softAPIP().toString() : WiFi.localIP().toString());
    response["minimumTemperature"] = boiler.getMinimumTemperature();
    
    String responseString;
    serializeJson(response, responseString);
    sendWebSocketResponse(num, responseString);
}