#ifndef LOCAL_NETWORK_H
#define LOCAL_NETWORK_H

#include <ArduinoJson.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WebSocketsServer.h>

class KiLL;

#include "Utils.h"
#include "Boiler.h"
#include "Display.h"

class LocalNetwork {
public:
    LocalNetwork(Boiler& boiler, Display& display);

    void initialize();
    void stopAccessPoint();

    void setupServer();
    void startServer();
    void stopServer();

    void setupLocalNetwork();

    const String getHostname();

private:
    static constexpr uint8_t MAX_MDNS_RETRIES = 10;
    static constexpr uint8_t WEBSOCKET_PORT = 81;
    
    WebSocketsServer webSocketServer;

    const String SSID();

    static void onStationConnected(WiFiEvent_t event, WiFiEventInfo_t info);
    static void onStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);

    Boiler& boiler;
    Display& display;

    /// @brief Handles WebSocket events
    void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length);
    /// @brief Processes incoming WebSocket messages
    void handleWebSocketMessage(uint8_t num, const String& message);
    /// @brief Sends response via WebSocket
    void sendWebSocketResponse(uint8_t num, const String& response);
    
    /// MARK: Message Handlers
    bool checkRequestData(JsonDocument& document, const String& message, const String source);
    void handleRootMessage(uint8_t num);
    void handleLocalMessage(uint8_t num);
    void handleSetupMessage(uint8_t num, JsonDocument& document);
    void handleResetFactoryMessage(uint8_t num, JsonDocument& document);
    void handleCommandMessage(uint8_t num, JsonDocument& document);
    void handleStatusMessage(uint8_t num, JsonDocument& document);

    /// MARK: Responses
    static const String STATUS_RESPONSE(const String message);
    static const String OK_RESPONSE;
    static const String ERROR_RESPONSE(const String message);
    static const String MISSING_AUTHENTICATION_RESPONSE;

    /// @brief Task handle for WebSocket server
    TaskHandle_t webSocketTaskHandle = NULL;
    /// @brief Static task function for WebSocket server
    static void webSocketTask(void* parameter);
};

#endif