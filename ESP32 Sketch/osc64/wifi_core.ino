#include <WiFi.h>
#include <freertos/message_buffer.h>
#include "common.h"
#include "wifi_core.h"

String macaddress = "";
String ssid = "empty";
String password = "empty";
String timeoffset = "empty";
String configured = "empty";
String myLocalIp = "0.0.0.0";
String romVersion = "0.0";
volatile bool pastMatrix = false;
MessageBufferHandle_t commandBuffer;
MessageBufferHandle_t responseBuffer;
bool isWifiCoreConnected = false;
unsigned long lastWifiBegin = 0;

void WifiCoreLoop(void* parameter) {
  WiFiCommandMessage commandMessage;
  WiFiResponseMessage responseMessage;

  for (;;) {
    size_t ret = xMessageBufferReceive(commandBuffer, &commandMessage, sizeof(commandMessage), pdMS_TO_TICKS(1000));

    if (ret != 0) {
      switch (commandMessage.command) {
        case WiFiBeginCommand:
          lastWifiBegin = millis();
          WiFi.mode(WIFI_STA);
          WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
          WiFi.begin(ssid.c_str(), password.c_str());
          break;
        case GetWiFiMacAddressCommand:
          responseMessage.command = GetWiFiMacAddressCommand;
          Network.macAddress().toCharArray(responseMessage.response.str, sizeof(responseMessage.response.str));
          xMessageBufferSend(responseBuffer, &responseMessage, sizeof(responseMessage), portMAX_DELAY);
          break;
        default:
          Serial.print("Invalid Command Message: ");
          Serial.println(commandMessage.command);
          break;
      }
    }

    isWifiCoreConnected = WiFi.isConnected();
    if (isWifiCoreConnected) {
      myLocalIp = WiFi.localIP().toString();
      continue;
    }

    myLocalIp = "0.0.0.0";
    if (millis() > lastWifiBegin + 7000) {
      lastWifiBegin = millis();
      WiFi.mode(WIFI_STA);
      WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
      WiFi.begin(ssid.c_str(), password.c_str());
    }
  }
}
