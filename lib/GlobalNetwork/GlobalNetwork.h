#ifndef GLOBAL_NETWORK_H
#define GLOBAL_NETWORK_H

#include <WebSocketsClient.h>
#include <WiFi.h>

#include "LocalNetwork.h"
#include "Memory.h"
#include "Utils.h"
#include "Boiler.h"

class GlobalNetwork {
public:
    GlobalNetwork(LocalNetwork& localNetwork, Boiler& boiler);

    /// @brief Connects to WiFi using credentials stored in memory
    void startWiFiConnection(const bool debug = true);
    /// @brief Tries to reconnect to WiFi if it is not connected and the memory is valid
    void tryReconnectWiFi();

    void keepConnectionWithServer();

    bool isConnectedToWifi();

private:
    const String serverURL = "smartboiler-server.onrender.com";
    static constexpr unsigned long WIFI_RETRY_INTERVAL = 60 * 1000;

    LocalNetwork& localNetwork;
    unsigned long lastWiFiAttempt;
    volatile bool wifiConnected;
    volatile bool lastMemoryStatus;
    volatile bool isConnectedToServer;

    Boiler& boiler;
    WebSocketsClient webSocket;

    /// @brief Handles WiFi events
    void onWiFiEvent(WiFiEvent_t event);
    /// @brief Handles WebSocket events
    void webSocketEvent(WStype_t type, uint8_t* payload, size_t length);
    void connectWebSocket();
};

#endif
