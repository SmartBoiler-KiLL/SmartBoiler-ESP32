#include "GlobalNetwork.h"

GlobalNetwork::GlobalNetwork(LocalNetwork& localNetwork, Boiler& boiler) : localNetwork(localNetwork), boiler(boiler) {
    lastWiFiAttempt = 0;
    wifiConnected = false;
    lastMemoryStatus = false;
    isConnectedToServer = false;

    WiFi.onEvent(std::bind(&GlobalNetwork::onWiFiEvent, this, std::placeholders::_1), WiFiEvent_t::ARDUINO_EVENT_MAX);
}

void GlobalNetwork::startWiFiConnection(const bool debug) {
    String ssid = Memory::getSSID();
    String password = Memory::getPassword();
    if (debug) Serial.println("[GlobalNetwork] Connecting to WiFi network: " + ssid);
    WiFi.begin(ssid, password);
    lastWiFiAttempt = millis();
}

bool GlobalNetwork::isConnectedToWifi() {
    return wifiConnected;
}

void GlobalNetwork::tryReconnectWiFi() {
    bool memoryStatus = Memory::verifyContent();
    if ((memoryStatus && !wifiConnected && millis() - lastWiFiAttempt > WIFI_RETRY_INTERVAL) || (memoryStatus && !lastMemoryStatus)) {
        lastWiFiAttempt = millis();
        Serial.println("[GlobalNetwork] Attempting to reconnect to WiFi network " + Memory::getSSID());
        startWiFiConnection(false);
    }

    lastMemoryStatus = memoryStatus;
}

void GlobalNetwork::keepConnectionWithServer() {
    webSocket.sendPing();
    webSocket.loop();    
}

void GlobalNetwork::onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_STA_CONNECTED:
            Serial.println("[GlobalNetwork] WiFi connected to network");
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            if (!Memory::verifyContent()) return;
            Serial.println("[GlobalNetwork] WiFi got IP address: " + WiFi.localIP().toString());
            wifiConnected = true;
            // localNetwork.stopAccessPoint();
            // connectWebSocket();
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            if (!wifiConnected) return;
            Serial.println("[GlobalNetwork] WiFi disconnected, enabling SoftAP and local server");
            wifiConnected = false;
            localNetwork.initialize();
            localNetwork.startServer();
            break;
    }
}

void GlobalNetwork::connectWebSocket() {
    webSocket.beginSSL(serverURL, 443, "/socket");
    webSocket.enableHeartbeat(15000, 3000, 2);
    webSocket.onEvent(std::bind(&GlobalNetwork::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    webSocket.setReconnectInterval(5000);
}

void GlobalNetwork::webSocketEvent(WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_CONNECTED: {
            if (isConnectedToServer) return;
            Serial.println("[GlobalNetwork] Connected to server via websocket");
            isConnectedToServer = true;
            break;
        }
        
        case WStype_DISCONNECTED: {
            if (!isConnectedToServer) return;
            Serial.println("[GlobalNetwork] Disconnected from server");
            isConnectedToServer = false;
            break;
        }
        
        case WStype_TEXT: {
            String command = String((char*)payload);
            JsonDocument document;
            DeserializationError error = deserializeJson(document, command);
            if (error) {
                Serial.println("[GlobalNetwork] Error parsing command: " + String(error.c_str()));
                return;
            }

            if (!Utils::verifyRequest(document)) return;

            // TODO: Handle command
            webSocket.sendTXT("{\"status\": \"OK\"}");
            break;
        }

        case WStype_PING: {
            Serial.println("[GlobalNetwork] Ping received from server");
            break;
        }

        case WStype_PONG: {
            // Serial.println("[GlobalNetwork] Pong received from server");
            break;
        }

        default: {
            Serial.println("[GlobalNetwork] Unhandled WebSocket event: " + String(type));
            break;
        }
    }
}