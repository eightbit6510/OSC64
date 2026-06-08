#ifndef WIFI_CORE_H
#define WIFI_CORE_H

#include "common.h"
#include <freertos/message_buffer.h>

extern String macaddress;
extern String ssid;
extern String password;
extern String timeoffset;
extern String myLocalIp;
extern String configured;
extern String romVersion;
extern volatile bool pastMatrix;
extern MessageBufferHandle_t commandBuffer;
extern MessageBufferHandle_t responseBuffer;
extern bool isWifiCoreConnected;

#define WiFiBeginCommand 1
#define GetWiFiMacAddressCommand 5

struct WiFiCommandMessage {
  byte command;
};

struct WiFiResponseMessage {
  byte command;
  union response {
    char str[20];
  } response;
};

void WifiCoreLoop(void* parameter);

#endif  // WIFI_CORE_H
