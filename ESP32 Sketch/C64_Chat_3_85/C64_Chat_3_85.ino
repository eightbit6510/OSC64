#include <Preferences.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <HTTPClient.h>
// NTP time includes removed - time is set manually on C64

#include "common.h"
#include "utils.h"
#include "wifi_core.h"
#include "prgfile.h"

Preferences settings;

//bool invert_reset_signal = false ;  // READ THIS:  If this settings is wrong, the program will never start because reset is hold down constant.
// set 'true' for pcb rev 2.0
// set 'false' for pcb rev 3.7
// set 'false' for pcb rev 3.8
//bool invert_nmi_signal = false;     // READ THIS:  If this settings is wrong, things might work but auto update will fail.
// set 'false' for pcb rev 2.0
// set 'true' for pcb rev 3.7,
// set 'false' for rev 3.8


// About the regID (registration id)
// A user needs to register at https://www.chat64.nl
// they will receive a registration_id via email.
// that number needs to be filled in on the account setup page on the commodore 64
// now the cartridge is registered and the administrator has a way to block the user
// the user can not register again with the same email address if they are blocked

// ********************************
// **     Global Variables       **
// ********************************
bool invert_reset_signal;
bool invert_nmi_signal;
String urgentMessage = "";
int wificonnected = -1;
char regStatus = 'u';
volatile bool dataFromC64 = false;
volatile bool io2 = false;
bool receivingBuffer = false;  // Flag to indicate we're receiving a buffer (not a command)
char inbuffer[250];  // a character buffer for incoming data
int inbuffersize = 0;
char outbuffer[250];  // a character buffer for outgoing data
char textsize = 0;
int it = 0;
int doReset = 0;
volatile byte ch = 0;
TaskHandle_t Task1;
byte send_error = 0;
int userpageCount = 0;
char multiMessageBufferPub[3500];
char multiMessageBufferPriv[3500];
unsigned long first_check = 0;
WiFiCommandMessage commandMessage;
WiFiResponseMessage responseMessage;
bool waitAck = false;
int scrollBufferIndex = 0;

// OSC variables
WiFiUDP Udp;
bool udpInitialized = false;
String oscServerIP = "192.168.178.244";
String oscServerPort = "8888";
String oscServerIP2 = "0.0.0.0";
String oscServerPort2 = "0";
String oscServerIP3 = "0.0.0.0";
String oscServerPort3 = "0";
String oscServerIP4 = "0.0.0.0";
String oscServerPort4 = "0";
String oscServerIP5 = "0.0.0.0";
String oscServerPort5 = "0";

// Tasmota device structure
struct TasmotaDevice {
  String name;
  String ip;
  String username;
  String password;
};

TasmotaDevice tasmotaDevices[10];
int tasmotaDeviceCount = 0;
// ********************************
// **        OUTPUTS             **
// ********************************
// see http://www.bartvenneker.nl/index.php?art=0030
// for usable io pins!

#define oC64D0 GPIO_NUM_5   // data bit 0 for data from the ESP32 to the C64
#define oC64D1 GPIO_NUM_33  // data bit 1 for data from the ESP32 to the C64
#define oC64D2 GPIO_NUM_14  // data bit 2 for data from the ESP32 to the C64
#define oC64D3 GPIO_NUM_23  // data bit 3 for data from the ESP32 to the C64
#define oC64D4 GPIO_NUM_13  // data bit 4 for data from the ESP32 to the C64
#define oC64D5 GPIO_NUM_19  // data bit 5 for data from the ESP32 to the C64
#define oC64D6 GPIO_NUM_18  // data bit 6 for data from the ESP32 to the C64
#define oC64D7 GPIO_NUM_26  // data bit 7 for data from the ESP32 to the C64

#define oC64RST GPIO_NUM_21  // reset signal to C64
#define oC64NMI GPIO_NUM_32  // non-maskable interrupt signal to C64
#define CLED GPIO_NUM_4      // led on cartridge
#define sclk GPIO_NUM_25     // serial clock signal to the shift register
#define pload GPIO_NUM_16    // parallel load signal to the shift register

// ********************************
// **        INPUTS              **
// ********************************
#define resetSwitch GPIO_NUM_15  // this pin outputs PWM signal at boot
#define C64IO1 GPIO_NUM_22
#define sdata GPIO_NUM_27
#define C64IO2 GPIO_NUM_17

// *************************************************
// Interrupt routine for IO1
// *************************************************
void IRAM_ATTR isr_io1() {
  // This signal goes LOW when the commodore writes to (or reads from) the IO1 address space
  // In our case the Commodore 64 only WRITES the IO1 address space, so ESP32 can read the data.
  digitalWrite(oC64D7, LOW);  // this pin is used for flow controll,
                              // make it low so the C64 will not send the next byte
                              // until we are ready for it
  ch = 0;
  digitalWrite(pload, HIGH);  // stop loading parallel data and enable shifting serial data
  ch = shiftIn(sdata, sclk, MSBFIRST);
  dataFromC64 = true;
  digitalWrite(pload, LOW);
}

// *************************************************
// Interrupt routine for IO2
// *************************************************
void IRAM_ATTR isr_io2() {
  // This signal goes LOW when the commodore reads from (or write to) the IO2 address space
  // In this case the commodore only uses the IO2 address space to read from, so ESP32 can send data.
  io2 = true;
}

// *************************************************
// Interrupt routine, to restart the esp32
// *************************************************
void IRAM_ATTR isr_reset() {
  ESP.restart();
}

void create_Task_WifiCore() {
  // we create a task for the second (unused) core of the esp32
  // this task will communicate with the web site while the other core
  // is busy talking to the C64
  xTaskCreatePinnedToCore(
    WifiCoreLoop, /* Function to implement the task */
    "Task1",      /* Name of the task */
    10000,        /* Stack size in words */
    NULL,         /* Task input parameter */
    0,            /* Priority of the task */
    &Task1,       /* Task handle. */
    0);           /* Core where the task should run */
}

// *************************************************
//  SETUP
// *************************************************
void setup() {
  Serial.begin(115200);

  commandBuffer = xMessageBufferCreate(sizeof(commandMessage) + sizeof(size_t));
  responseBuffer = xMessageBufferCreate(sizeof(responseMessage) + sizeof(size_t));

  create_Task_WifiCore();

  // get the wifi mac address, this is used to identify the cartridge.
  commandMessage.command = GetWiFiMacAddressCommand;
  xMessageBufferSend(commandBuffer, &commandMessage, sizeof(commandMessage), portMAX_DELAY);
  xMessageBufferReceive(responseBuffer, &responseMessage, sizeof(responseMessage), portMAX_DELAY);
  macaddress = responseMessage.response.str;
  macaddress.replace(":", "");
  macaddress.toLowerCase();
  macaddress = macaddress.substring(4);

  // add a checksum to the mac address.
  byte data[4];
  int i = 0;
  for (unsigned int t = 0; t < macaddress.length(); t = t + 2) {
    String p = macaddress.substring(t, t + 2);
    char n[3];
    p.toCharArray(n, 3);
    byte f = x2i(n);
    data[i++] = f;
  }
  String crc8 = String(checksum(data, 4), HEX);

  if (crc8.length() == 1) crc8 = "0" + crc8;  // add a leading zero if the result is 1 digit.
  macaddress += crc8;

  // init settings object to store settings in the eeprom
  settings.begin("mysettings", false);
  //
  doReset = settings.getInt("doReset", 0);
  // get the configured status from the eeprom
  configured = settings.getString("configured", "empty");

  // get the registration id from the eeprom
  regID = settings.getString("regID", "unregistered!");

  // get the nick name from the eeprom
  myNickName = settings.getString("myNickName", "empty");
  lastprivmsg = settings.getULong("lastprivmsg", 1);       // get the last known message id (only the private is stored in eeprom)
  server = settings.getString("server", "www.chat64.nl");  // get Chatserver ip/fqdn from eeprom
  ssid = settings.getString("ssid", "empty");              // get WiFi credentials and Chatserver ip/fqdn from eeprom
  password = settings.getString("password", "empty");
  timeoffset = settings.getString("timeoffset", "+0");  // get the time offset from the eeprom

  // Load OSC IP/port pairs from EEPROM
  oscServerIP = settings.getString("oscServerIP", "192.168.178.244");
  oscServerPort = settings.getString("oscServerPort", "8888");
  oscServerIP2 = settings.getString("oscServerIP2", "0.0.0.0");
  oscServerPort2 = settings.getString("oscServerPort2", "0");
  oscServerIP3 = settings.getString("oscServerIP3", "0.0.0.0");
  oscServerPort3 = settings.getString("oscServerPort3", "0");
  oscServerIP4 = settings.getString("oscServerIP4", "0.0.0.0");
  oscServerPort4 = settings.getString("oscServerPort4", "0");
  oscServerIP5 = settings.getString("oscServerIP5", "0.0.0.0");
  oscServerPort5 = settings.getString("oscServerPort5", "0");

  // Load Tasmota device count
  tasmotaDeviceCount = settings.getInt("tasmotaCount", 0);
  // Load Tasmota devices (max 10)
  for (int i = 0; i < tasmotaDeviceCount && i < 10; i++) {
    String prefix = "tasmota" + String(i);
    tasmotaDevices[i].name = settings.getString((prefix + "name").c_str(), "");
    tasmotaDevices[i].ip = settings.getString((prefix + "ip").c_str(), "");
    tasmotaDevices[i].username = settings.getString((prefix + "user").c_str(), "");
    tasmotaDevices[i].password = settings.getString((prefix + "pass").c_str(), "");
  }

  if (settings.isKey("invRST"))  // if this key exists, use it
    invert_reset_signal = settings.getInt("invRST");
  else {  // key does not exist, so create it
    invert_reset_signal = false;
    settings.putInt("invRST", 0);
  }

  if (settings.isKey("invNMI"))  // if this key exists, use it
    invert_nmi_signal = settings.getInt("invNMI", 0);
  else {  // key does not exist, so create it
    invert_nmi_signal = false;
    settings.putInt("invNMI", 0);
  }

  settings.end();

  // define inputs
  pinMode(sdata, INPUT);
  pinMode(C64IO1, INPUT_PULLDOWN);
  pinMode(C64IO2, INPUT_PULLUP);
  pinMode(resetSwitch, INPUT_PULLUP);

  // define interrupts
  attachInterrupt(C64IO1, isr_io1, RISING);          // interrupt for io1, C64 writes data to io1 address space
  attachInterrupt(C64IO2, isr_io2, FALLING);         // interrupt for io2, c64 reads
  attachInterrupt(resetSwitch, isr_reset, FALLING);  // interrupt for reset button

  // define outputs
  pinMode(CLED, OUTPUT);
  digitalWrite(CLED, LOW);
  pinMode(oC64D0, OUTPUT);
  pinMode(oC64D1, OUTPUT);
  pinMode(oC64D2, OUTPUT);
  pinMode(oC64D3, OUTPUT);
  pinMode(oC64D4, OUTPUT);
  pinMode(oC64D5, OUTPUT);
  pinMode(oC64D6, OUTPUT);
  pinMode(oC64D7, OUTPUT);
  digitalWrite(oC64D7, LOW);
  pinMode(oC64RST, OUTPUT);
  pinMode(oC64NMI, OUTPUT);
  digitalWrite(oC64RST, invert_reset_signal);
  digitalWrite(oC64NMI, invert_nmi_signal);
  pinMode(pload, OUTPUT);
  digitalWrite(pload, LOW);  // must be low to load parallel data
  pinMode(sclk, OUTPUT);
  digitalWrite(sclk, LOW);  // data shifts to serial data output on the transition from low to high.

  if (doReset != 157) {                           // this is used to reset the ESP32 without resetting the C64 (157 is a random number)
    digitalWrite(oC64RST, !invert_reset_signal);  // Reset the C64, toggle the output pin
    delay(250);
    digitalWrite(oC64RST, invert_reset_signal);
  } else {
    settings.begin("mysettings", false);
    settings.putInt("doReset", 0);
    messageIds[0] = settings.getULong("lastPubMessage", 0);
    messageIds[1] = settings.getULong("lastPrivMessage", 0);
    settings.end();
    pastMatrix = true;
  }
  settings.begin("mysettings", false);
  settings.putInt("doReset", 0);
  settings.end();

  // load the prg file
  if (!pastMatrix) loadPrgfile();

  // start wifi
  commandMessage.command = WiFiBeginCommand;
  xMessageBufferSend(commandBuffer, &commandMessage, sizeof(commandMessage), portMAX_DELAY);

  // check if we are connected to wifi
  if (isWifiCoreConnected) {
    wificonnected = 1;
    // Initialize UDP for OSC
    Udp.begin(8888);
    udpInitialized = true;
    
    // NTP time configuration removed - time is set manually on C64
    
#ifdef debug
    Serial.print("Macaddress=");
    Serial.println(macaddress);
    Serial.print("Connected to WiFi network with IP Address: ");
    Serial.println(myLocalIp);
    Serial.print("Name of the server (from eeprom): ");
    Serial.println(server);
#endif

  } else {
    // if there is no wifi, the user can change the credentials in cartridge menu
#ifdef debug
    Serial.println("NO Wifi connection!");
#endif
  }

}  // end of setup

// ******************************************************************************
// Main loop
// ******************************************************************************
unsigned long ledtimer = 0;
int privateBufferIndex = 0;
int publicBufferIndex = 0;
bool wifiError = false;

void loop() {
  digitalWrite(CLED, isWifiCoreConnected);
  digitalWrite(oC64D7, HIGH);  // Ready to receive!

  if (isWifiCoreConnected and wificonnected == -1) wificonnected = 1;  // only set wificonnected if it has not been set
  
  // If we're receiving a buffer, skip all command processing and let receive_buffer_from_C64 handle it
  if (receivingBuffer) {
    delayMicroseconds(10);  // Small delay to allow receive_buffer_from_C64 to process bytes
    return;  // Skip rest of loop
  }
  
  if (dataFromC64) {
      // Normal command processing
      dataFromC64 = false;
      digitalWrite(oC64D7, LOW);  // flow control
#ifdef debug
      Serial.printf("incoming command: %d\n", ch);
#endif
    if (wifiError and isWifiCoreConnected) {
      urgentMessage = "[grn]Wifi connection restored.               ";
      wifiError = false;
      wificonnected = 1;
    }

    // generate an error if wifi connection drops
    if (wificonnected == 1 && !isWifiCoreConnected) {
      digitalWrite(CLED, LOW);
      wificonnected = 0;
      myLocalIp = "0.0.0.0";
      urgentMessage = "[red]Error in WiFi connection, please check  ";
      wifiError = true;
    }

    // 254 = C64 triggers call to the website for new public message
    // 253 = new chat message from c64 to database
    // 252 = C64 sends the new wifi network name (ssid) AND password AND time offset
    // 251 = C64 ask for the current wifi ssid,password and time offset
    // 250 = C64 ask for the first full page of messages (during startup)
    // 249 = get result of last send action (253)
    // 248 = C64 ask for the wifi connection status
    // 247 = C64 triggers call to the website for new private message
    // 246 = set chatserver ip/fqdn
    // 245 = check if the esp is connected at all, or are we running in simulation mode?
    // 244 = reset to factory defaults
    // 243 = C64 ask for the mac address, registration id, nickname and regstatus
    // 242 = do update
    // 241 = get the number of unread private messages
    // 240 = C64 sends the new registration id and nickname to ESP32
    // 239 = C64 asks if updated firmware is available
    // 238 = C64 triggers call to the chatserver to test connectivity
    // 237 = get chatserver connectivity status
    // 236 = C64 asks for the server configuration status and servername
    // 235 = C64 sends server configuration status
    // 234 = get user list first page
    // 233 = get user list next page
    // 232 =
    // 231 =
    // 230 = get current time from internet (NTP) and send to C64
    // 229 = scroll up or down
    // 228 = set OSC server IP and port
    // 227 = get OSC server IP and port
    // 226 = send OSC message
    // 225 = save unified config (OSC, Tasmota, cues)
    // 224 = load unified config
    // 223 = get Tasmota device list
    // 222 = save Tasmota device
    // 221 = save cue list
    // 220 = load cue list
    //
    // 128 = end marker, ignore

    switch (ch) {
      case 254:
        // ------------------------------------------------------------------------------
        // start byte 254 = C64 triggers call to the website for new public message
        // ------------------------------------------------------------------------------
        if (first_check == 0) first_check = millis();
        doUrgentMessage();  // send urgent messages first

        // if the user list is empty, get the list
        // also refresh the userlist when we switch from public to private messaging and vice versa
        if (onLineUsers.length() < 1 or msgtype != "public") updateUserlist = true;
        msgtype = "public";

        if (getMessageFromMMBuffer(multiMessageBufferPub, &publicBufferIndex, false)) {
          if (Deserialize() == 1) {
            // deserialize returns 1 of there was a message in the buffer (2 for private message).
            send_buffer_to_C64(msgbuffer, msgbuffersize);  // send it
            messageIds[0] = tempMessageIds[0];             // store the new message id;
          } else sendByte(128);                            // no more messages for now.
        } else {
          sendByte(128);  // no more messages for now.
          getMessage = true;
        }
        break;
      case 253:
        {
          // ------------------------------------------------------------------------------
          // start byte 253 = new chat message from c64 to database
          // ------------------------------------------------------------------------------

          // we expect a chat message from the C64
          receive_buffer_from_C64(1);
          String toEncode = "";
          String RecipientName = "";
          int mstart = 0;
          String colorCode = "[145]";
          // Get the RecipientName
          // see if the message starts with '@'
          byte b = inbuffer[1];
          if (b == 0) {
            toEncode = "[" + String(int(inbuffer[0])) + "]";
            for (int x = 2; x < 15; x++) {
              byte b = inbuffer[x];
              if (b != 32) {
                if (b < 127) {
                  RecipientName = (RecipientName + screenCode_to_Ascii(b));
                } else {
                  colorCode = "[" + String(int(b)) + "]";
                }
              } else {
                mstart = x + 1;
                toEncode = toEncode + "@" + RecipientName + " " + colorCode;
                break;
              }
            }
          }

          for (int x = mstart; x < inbuffersize; x++) {
            inbuffer[x] = screenCode_to_Ascii(inbuffer[x]);
            byte b = inbuffer[x];
            if (b > 128) {
              toEncode = (toEncode + "[" + int(inbuffer[x]) + "]");
            } else {
              toEncode = (toEncode + inbuffer[x]);
            }
          }

          if (RecipientName != "") {
            // is this a valid username?
            String test_name = RecipientName;
            test_name.toLowerCase();
#ifdef debug
            Serial.print("known users: ");
            Serial.println(onLineUsers);
            Serial.println(offLineUsers);
            Serial.print("Name under test: ");
            Serial.println(test_name);
#endif
            if ((offLineUsers.indexOf(test_name + ';') >= 0) or (onLineUsers.indexOf(test_name + ';') >= 0)) {
              // user exists
              msgtype = "private";
              pmSender = '@' + RecipientName;
            } else {
              // user does not exist
              urgentMessage = "[red]System:  Unknown user:" + test_name;
              send_error = 1;
              break;
            }
          } else {
            msgtype = "public";
          }

          int buflen = toEncode.length() + 1;
          char buff[buflen];
          toEncode.toCharArray(buff, buflen);
          String Encoded = my_base64_encode(buff, buflen);
          Encoded = urlEncode(Encoded);
          // Now send it with retry!
          bool sc = false;
          int retry = 0;
          while (sc == false and retry < 2) {
            sendingMessage = 1;
            commandMessage.command = SendMessageToServerCommand;
            Encoded.toCharArray(commandMessage.data.sendMessageToServer.encoded, sizeof(commandMessage.data.sendMessageToServer.encoded));
            RecipientName.toCharArray(commandMessage.data.sendMessageToServer.recipientName, sizeof(commandMessage.data.sendMessageToServer.recipientName));
            commandMessage.data.sendMessageToServer.retryCount = retry;
            xMessageBufferSend(commandBuffer, &commandMessage, sizeof(commandMessage), portMAX_DELAY);
            xMessageBufferReceive(responseBuffer, &responseMessage, sizeof(responseMessage), portMAX_DELAY);
            sc = responseMessage.response.boolean;
            // sending the message fails, take a short break and try again
            if (!sc) {
              delay(1000);
              retry = retry + 1;
            }
          }
          // if it still fails after a few retries, give us an error.
          if (!sc) {
            urgentMessage = "[red]  System:     ERROR sending the message";
            send_error = 1;
          } else {
            // No error, read the message back from the database to show it on screen
            sendingMessage = 0;
            getMessage = true;  // get the message we just inserted
          }
          break;
        }
      case 252:
        {
          // ------------------------------------------------------------------------------
          // 252 = C64 sends the new wifi network name (ssid) AND password AND time offset
          // ------------------------------------------------------------------------------
          receive_buffer_from_C64(3);

          for (int x = 0; x < inbuffersize; x++) {
            inbuffer[x] = screenCode_to_Ascii(inbuffer[x]);
          }

          // inbuffer now contains "SSID password timeoffset"
          char bns[inbuffersize + 1];
          strncpy(bns, inbuffer, inbuffersize + 1);
          String ns = bns;

          ssid = getValue(ns, 129, 0);
          ssid.trim();
          Serial.println(ssid);
          password = getValue(ns, 129, 1);
          password.trim();
          Serial.println(password);
          timeoffset = getValue(ns, 129, 2);
          timeoffset.trim();
          Serial.println(timeoffset);

          settings.begin("mysettings", false);
          settings.putString("ssid", ssid);
          settings.putString("password", password);
          settings.putString("timeoffset", timeoffset);
          settings.end();
          Serial.println(1);
          softReset(0);
          //commandMessage.command = WiFiBeginCommand;
          //xMessageBufferSend(commandBuffer, &commandMessage, sizeof(commandMessage), portMAX_DELAY);
          break;
        }
      case 251:
        // ------------------------------------------------------------------------------
        // start byte 251 = C64 ask for the current wifi ssid,password and time offset
        // ------------------------------------------------------------------------------
        send_String_to_c64(ssid + char(129) + password + char(129) + timeoffset);
        break;
      case 249:
        // ------------------------------------------------------------------------------
        // start byte 249 = C64 asks if this is an existing user (for private chat)
        // ------------------------------------------------------------------------------
        sendByte(send_error);
        sendByte(128);
        send_error = 0;
        break;
      case 248:
        // ------------------------------------------------------------------------------
        // start byte 248 = C64 ask for the wifi connection status
        // ------------------------------------------------------------------------------

        if (!isWifiCoreConnected) {
          digitalWrite(CLED, LOW);
          sendByte(146);
          send_String_to_c64("Not Connected to Wifi");
        } else {
          wificonnected = 1;
          digitalWrite(CLED, HIGH);
          sendByte(149);
          String wifi_status = "Connected with WiFi!";
          if (myLocalIp != "0.0.0.0") wifi_status = "Connected with ip " + myLocalIp;
          send_String_to_c64(wifi_status);
        }
        break;
      case 247:
        // ------------------------------------------------------------------------------
        // start byte 247 = C64 triggers call to the website for new private message
        // ------------------------------------------------------------------------------

        // send urgent messages first
        doUrgentMessage();
        // if the user list is empty, get the list
        // also refresh the userlist when we switch from public to private messaging and vice versa
        if (onLineUsers.length() < 1 or msgtype != "private") updateUserlist = true;
        msgtype = "private";
        if (getMessageFromMMBuffer(multiMessageBufferPriv, &privateBufferIndex, true)) {
          if (Deserialize() == 2) {
            // deserialize returns 2 of there was a private message in the buffer
            //for (int x = 0; x < msgbuffersize; x++) outbuffer[x] = msgbuffer[x];

            send_buffer_to_C64(msgbuffer, msgbuffersize);  // send it
            messageIds[1] = tempMessageIds[1];             // store the new message id;
            lastprivmsg = messageIds[1];
            settings.begin("mysettings", false);
            settings.putULong("lastprivmsg", messageIds[1]);  // store message id in eeprom
            settings.end();
          } else sendByte(128);  // no more messages for now.
        } else {
          sendByte(128);  // no more messages for now.
          getMessage = true;
        }
        break;
      case 246:
        {
          // ------------------------------------------------------------------------------
          // start byte 246 = C64 sends a new chat server ip/fqdn
          // ------------------------------------------------------------------------------

          receive_buffer_from_C64(1);
          for (int x = 0; x < inbuffersize; x++) {
            inbuffer[x] = screenCode_to_Ascii(inbuffer[x]);
          }

          char bns[inbuffersize + 1];
          strncpy(bns, inbuffer, inbuffersize + 1);
          String ns = bns;
          ns.trim();
          server = ns;
          settings.begin("mysettings", false);
          settings.putString("server", ns);  // store the new server name in the eeprom settings
          settings.end();

          messageIds[0] = 0;
          messageIds[1] = 0;

          // we should also refresh the userlist
          onLineUsers = "";
          break;
        }
      case 245:
        {
          // -----------------------------------------------------------------------------------------------------
          // start byte 245 = C64 checks if the Cartrdidge is connected at all.. or are we running in a simulator?
          // -----------------------------------------------------------------------------------------------------
          // receive the ROM version number
          bool rxError;
          //delay(100);
          rxError = receive_buffer_from_C64(1);
          if (rxError) configured = "fail";
          if (!rxError and configured == "fail") {
            settings.begin("mysettings", false);
            configured = settings.getString("configured", "empty");
            settings.end();
          }

          char bns[inbuffersize + 1];
          // filter out any unwanted bytes, keep only ./01234567890
          for (int k = 0; k < inbuffersize; k++) {
            if (inbuffer[k] < 45 or inbuffer[k] > 57) inbuffer[k] = 32;
          }
          strncpy(bns, inbuffer, inbuffersize + 1);
          String ns = bns;
          ns.replace(" ", "");
          romVersion = ns;
          // respond with byte 128 to tell the commodore the cartridge is present
          sendByte(128);

          if (!rxError) {
            pastMatrix = true;
            getMessage = true;
          }
#ifdef debug
          Serial.println("ROM Version=" + romVersion);
          Serial.println("Are we in the Matrix?");
#endif
          break;
        }
      case 244:
        {
          // ---------------------------------------------------------------------------------
          // start byte 244 = C64 sends the command to reset the cartridge to factory defaults
          // ---------------------------------------------------------------------------------
          // this will reset all settings
          //Serial.println("__reset");
          receive_buffer_from_C64(1);
          char bns[inbuffersize + 1];
          strncpy(bns, inbuffer, inbuffersize + 1);
          String ns = bns;
          if (ns.startsWith("RESET!")) {
            settings.begin("mysettings", false);
            settings.putString("regID", "unregistered!");
            settings.putString("myNickName", "empty");
            settings.putString("ssid", "empty");
            settings.putString("password", "empty");
            settings.putString("server", "www.chat64.nl");
            settings.putString("configured", "empty");
            settings.putString("timeoffset", "+0");
            settings.end();
            // now reset the esp
            ESP.restart();
          }
          break;
        }
      case 243:
        {
          // ------------------------------------------------------------------------------
          // start byte 243 = C64 ask for the mac address, registration id, nickname and regstatus
          // ------------------------------------------------------------------------------
          commandMessage.command = GetRegistrationStatusCommand;
          xMessageBufferSend(commandBuffer, &commandMessage, sizeof(commandMessage), portMAX_DELAY);
          xMessageBufferReceive(responseBuffer, &responseMessage, sizeof(responseMessage), portMAX_DELAY);
          regStatus = responseMessage.response.str[0];
          send_String_to_c64(macaddress + char(129) + regID + char(129) + myNickName + char(129) + regStatus);
          break;
        }
      case 242:
        {
          // ------------------------------------------------------------------------------
          // start byte 242 = Do the actual update!
          // ------------------------------------------------------------------------------
          receive_buffer_from_C64(1);
          char bns[inbuffersize + 1];
          strncpy(bns, inbuffer, inbuffersize + 1);
          String ns = bns;
          if (ns.startsWith("UPDATE!")) {
            outByte(1);
            detachInterrupt(C64IO1);  // disable IO1 and IO2 interrupts
            detachInterrupt(C64IO2);  // disable IO1 and IO2 interrupts
            commandMessage.command = DoUpdateCommand;
            xMessageBufferSend(commandBuffer, &commandMessage, sizeof(commandMessage), portMAX_DELAY);
          }
          break;
        }
      case 241:
        {
          // ------------------------------------------------------------------------------
          // start byte 241 = C64 asks for the number of unread private messages
          // ------------------------------------------------------------------------------
          if (pmCount > 10) pmCount = 10;
          String pm = String(pmCount);
          if (pmCount < 10) { pm = "0" + pm; }
          pm = "[pm:" + pm + " (F5)]";
          if (pmCount == 0) pm = "~~~~~~~~~~~~";
          sendByte(156);           // send color code for gray
          send_String_to_c64(pm);  // then send the number of messages as a string
          break;
        }
      case 240:
        {
          // ------------------------------------------------------------------------------
          // start byte 240 = C64 sends the new registration id and nickname to ESP32
          // ------------------------------------------------------------------------------
          receive_buffer_from_C64(2);
          for (int x = 0; x < inbuffersize; x++) {
            inbuffer[x] = screenCode_to_Ascii(inbuffer[x]);
          }
          // inbuffer now contains "registrationid nickname"
          char bns[inbuffersize + 1];
          strncpy(bns, inbuffer, inbuffersize + 1);
          String ns = bns;

          regID = getValue(ns, 129, 0);
          regID.trim();
#ifdef debug
          Serial.println(regID);
#endif
          if (regID.length() != 16) {
#ifdef debug
            Serial.println("Registration code length should be 16");
#endif
            break;
          }
          myNickName = getValue(ns, 129, 1);
          myNickName.trim();
          myNickName.replace(' ', '_');
#ifdef debug
          Serial.println(myNickName);
#endif

          settings.begin("mysettings", false);
          settings.putString("regID", regID);
          settings.putString("myNickName", myNickName);
          settings.end();
          break;
        }
      case 239:
        {
          String s = newVersions;
          if (millis() < (first_check + 10000)) s = "";
          send_String_to_c64(s);
          break;
        }
      case 238:
        {
          // ------------------------------------------------------------------------------
          // start byte 238 = C64 triggers call to the chatserver to test connectivity
          // ------------------------------------------------------------------------------
          ServerConnectResult = "Timeout in response, try again";
          commandMessage.command = ConnectivityCheckCommand;
          xMessageBufferSend(commandBuffer, &commandMessage, sizeof(commandMessage), portMAX_DELAY);
          break;
        }
      case 237:
        {
          // ------------------------------------------------------------------------------
          // start byte 237 = C64 triggers call to receive connection status
          // ------------------------------------------------------------------------------
          sendByte(ResultColor);  // send color code for green if connected
          send_String_to_c64(ServerConnectResult);
          break;
        }
      case 236:
        {
          // ------------------------------------------------------------------------------
          // start byte 236 = C64 asks for the server configuration status and servername
          // ------------------------------------------------------------------------------
          if (!pastMatrix) configured = "fail";  // if the matrix chack has failed, we corrupt the config value
                                                 // this will force the cartridge to try again.

          send_String_to_c64(configured + char(129) + server + char(129) + SwVersion);
#ifdef debug
          Serial.println("response 236 = " + configured + " " + server + " " + SwVersion);
#endif

          if (configured == "fail") {
            settings.begin("mysettings", false);  // restore the config value if needed
            configured = settings.getString("configured", "empty");
            settings.end();
          }
          break;
        }
      case 235:
        {
          // ------------------------------------------------------------------------------
          // start byte 235 = C64 sends server configuration status
          // ------------------------------------------------------------------------------

          receive_buffer_from_C64(1);
          for (int x = 0; x < inbuffersize; x++) {
            inbuffer[x] = screenCode_to_Ascii(inbuffer[x]);
          }

          char bns[inbuffersize + 1];
          strncpy(bns, inbuffer, inbuffersize + 1);
          String ns = bns;
          configured = ns;
          settings.begin("mysettings", false);
          settings.putString("configured", ns);
          settings.end();
          break;
        }
      case 234:
        {
          // c64 asks for user list, first page.
          // we send a max of 20 users in one long string
          userpageCount = 0;
          String ul1 = userPages[userpageCount];
          send_String_to_c64(ul1);
          userpageCount++;
          break;
        }
      case 233:
        {
          // c64 asks for user list, second or third page.
          // we send a max of 20 users in one long string
          String ul1 = userPages[userpageCount];
          send_String_to_c64(ul1);
          userpageCount++;
          break;
        }
      case 230:
        {
          // ------------------------------------------------------------------------------
          // start byte 230 = (Unused - NTP time removed, time is set manually on C64)
          // ------------------------------------------------------------------------------
          sendByte(128);  // Send error marker (not implemented)
          break;
        }
      case 229:
        {
          // ------------------------------------------------------------------------------
          // start byte 229 = Scroll Up or Down
          // ------------------------------------------------------------------------------
          static String decoded_message;
          receive_buffer_from_C64(1);
          if (inbuffer[0] == 0) topMes = 0;
          systemLineCount = inbuffer[1];
          pageSize = inbuffer[2];
          scrollDirection = inbuffer[3];  // 1 = up, 0 = down

          Serial.println("Scrolling starts");
          if (waitAck) {
            // if waitAck is true we repeat the message
            Serial.println("Waiting for ACK, resending last message");
            send_String_to_c64(decoded_message);
            break;
          }
          if (scrollBufferIndex == 0) {  // send a message to the other core to collect older messages
            gotScrollMessage = 0;
            commandMessage.command = ScrollUpDown;
            xMessageBufferSend(commandBuffer, &commandMessage, sizeof(commandMessage), portMAX_DELAY);
            while (gotScrollMessage == 0) delay(10);  // wait for that procedure to complete
          }
          // (more) data is in multiMessageBufferPub, collect a single message from the MMB
          if (getMessageFromMMBuffer(multiMessageBufferPub, &scrollBufferIndex, false)) {
            if (Deserialize() == 3) {  // decoded message is in the msgbuffer now
              if (scrollDirection == 1) topMes = tempMessageIds[2];
              if (scrollDirection == 0) botMes = tempMessageIds[2];
              Serial.println("Wait for ACK now");
              waitAck = true;
              decoded_message = String(msgbuffer, msgbuffersize);
              send_buffer_to_C64(msgbuffer, msgbuffersize);  // send it
              if (scrollDirection == 1 and topMes != 0) botMes = 0;
              break;
            }
          }
          sendByte(128);
          waitAck = false;
          break;
        }
      case 227:
        {
          // ------------------------------------------------------------------------------
          // start byte 227 = C64 asks for OSC server IP and port
          // ------------------------------------------------------------------------------
          // Combine all IPs and ports into a single string: ip1[129]port1[129]ip2[129]port2[129]...
          // Using char(129) as delimiter for all fields since splitRXbuffer uses that
          String response = 
              oscServerIP + char(129) + oscServerPort + char(129) +
              oscServerIP2 + char(129) + oscServerPort2 + char(129) +
              oscServerIP3 + char(129) + oscServerPort3 + char(129) +
              oscServerIP4 + char(129) + oscServerPort4 + char(129) +
              oscServerIP5 + char(129) + oscServerPort5;

          send_String_to_c64(response);
          break;
        }

      case 226:
        {
          // ------------------------------------------------------------------------------
          // start byte 226 = C64 sends OSC message to send
          // ------------------------------------------------------------------------------
          // Set flag BEFORE any checks to prevent buffer bytes from being processed as commands
          receivingBuffer = true;
          
          if (!isWifiCoreConnected) {
            Serial.println("[226] ERROR: WiFi not connected");
            // Still need to receive and discard the buffer
            receive_buffer_from_C64(1);
            receivingBuffer = false;
            sendByte(128);
            break;
          }
          
          // Initialize UDP if not already initialized
          if (!udpInitialized) {
            Serial.println("[226] Initializing UDP...");
            if (Udp.begin(8888)) {
              udpInitialized = true;
              Serial.println("[226] UDP initialized successfully");
            } else {
              Serial.println("[226] ERROR: Failed to initialize UDP");
              // Still need to receive and discard the buffer
              receive_buffer_from_C64(1);
              receivingBuffer = false;
              sendByte(128);
              break;
            }
          }

          receive_buffer_from_C64(1);
          receivingBuffer = false;  // Clear flag AFTER receiving buffer is complete
          
          Serial.print("[226] Received buffer size: ");
          Serial.println(inbuffersize);
          
          char oscMsg[256];
          
          // Convert and null-terminate, stopping at end marker (128)
          int i;
          for (i = 0; i < inbuffersize && i < 255; i++) {
            if (inbuffer[i] == 128) {  // Stop at end marker
              break;
            }
            oscMsg[i] = screenCode_to_Ascii(inbuffer[i]);
          }
          oscMsg[i] = '\0';  // Null-terminate the string

          Serial.print("[226] Converted message: '");
          Serial.print(oscMsg);
          Serial.print("' (length: ");
          Serial.print(i);
          Serial.println(")");

          // Check if this is a Tasmota command (tasmota: or tas: prefix)
          String oscMsgStr = String(oscMsg);
          oscMsgStr.trim();  // Remove any leading/trailing whitespace
          if (oscMsgStr.startsWith("tasmota:") || oscMsgStr.startsWith("tas:")) {
            // Extract device/group name and command
            int prefixLen = oscMsgStr.startsWith("tasmota:") ? 8 : 4;
            int spacePos = oscMsgStr.indexOf(' ', prefixLen);
            String targetName;
            String command;
            
            if (spacePos > 0) {
              // Format: "tasmota:device command" - extract both device and command
              targetName = oscMsgStr.substring(prefixLen, spacePos);
              targetName.trim();
              command = oscMsgStr.substring(spacePos + 1);
              command.trim();
            } else {
              // Format: "tasmota:device" - device name only, no command
              targetName = oscMsgStr.substring(prefixLen);
              targetName.trim();
              command = "";  // Empty command
            }
              
            bool found = false;
            // Find device(s) and send HTTP request
            // Support both exact device name match and group name match
            // Check all 10 device slots, not just tasmotaDeviceCount
            for (int j = 0; j < 10; j++) {
              // Skip empty device slots
              if (tasmotaDevices[j].name.length() == 0 || tasmotaDevices[j].ip.length() == 0) {
                continue;
              }
              
              String deviceName = tasmotaDevices[j].name;
              deviceName.trim();
              
              // Parse device name to extract base name and group (if comma exists)
              int commaPos = deviceName.indexOf(',');
              String baseName;
              String groupName = "";
              if (commaPos > 0) {
                baseName = deviceName.substring(0, commaPos);
                baseName.trim();
                groupName = deviceName.substring(commaPos + 1);
                groupName.trim();
              } else {
                baseName = deviceName;
              }
              
              // Match by exact device name (before comma) or by group name
              if (baseName.equalsIgnoreCase(targetName) || 
                  (groupName.length() > 0 && groupName.equalsIgnoreCase(targetName))) {
                Serial.print("[226] Tasmota match: device='");
                Serial.print(baseName);
                Serial.print("', IP='");
                Serial.print(tasmotaDevices[j].ip);
                Serial.print("', command='");
                Serial.print(command);
                Serial.println("'");
                // Send Tasmota HTTP command (even if command is empty, sendTasmotaCommand should handle it)
                sendTasmotaCommand(tasmotaDevices[j].ip, command, 
                                   tasmotaDevices[j].username, 
                                   tasmotaDevices[j].password);
                found = true;
                // Continue loop to handle group matches (multiple devices can be in same group)
              }
            }
            
            if (!found) {
              Serial.print("[226] Tasmota device not found: '");
              Serial.print(targetName);
              Serial.println("'");
            }
            
            sendByte(128);  // Always send acknowledgment
            break;
          }

          // Regular OSC message - parse and send to all configured servers
          // Define all IP/port pairs
          String ipPortPairs[5][2] = {
            {oscServerIP, oscServerPort},
            {oscServerIP2, oscServerPort2},
            {oscServerIP3, oscServerPort3},
            {oscServerIP4, oscServerPort4},
            {oscServerIP5, oscServerPort5}
          };

          Serial.print("[226] Processing OSC message: '");
          Serial.print(oscMsg);
          Serial.println("'");

          // Parse OSC address and arguments
          char oscAddress[256];
          char oscArgs[256] = "";
          char* addressEnd = strchr(oscMsg, ' ');
          if (!addressEnd) {
            // No arguments - just the address
            strncpy(oscAddress, oscMsg, sizeof(oscAddress) - 1);
            oscAddress[sizeof(oscAddress) - 1] = '\0';
            oscArgs[0] = '\0';
          } else {
            // Has arguments - split address and arguments
            int addrLen = addressEnd - oscMsg;
            strncpy(oscAddress, oscMsg, addrLen);
            oscAddress[addrLen] = '\0';
            strncpy(oscArgs, addressEnd + 1, sizeof(oscArgs) - 1);
            oscArgs[sizeof(oscArgs) - 1] = '\0';
          }

          // Parse arguments into a list (so we can reuse for all servers)
          // We'll parse as we go for each server, but need to work with a copy
          char argsCopy[256];
          
          // Send to all valid IP/port pairs
          for (int j = 0; j < 5; j++) {
            if (ipPortPairs[j][0] != "0.0.0.0" && ipPortPairs[j][1] != "0") {
              OSCMessage msg(oscAddress);
              
              // If there are arguments, parse and add them
              if (oscArgs[0] != '\0') {
                strncpy(argsCopy, oscArgs, sizeof(argsCopy) - 1);
                argsCopy[sizeof(argsCopy) - 1] = '\0';
                
                char* token = strtok(argsCopy, " ");
                while (token != nullptr) {
                  // Try to parse as int
                  char* endptr;
                  long intVal = strtol(token, &endptr, 10);
                  if (*endptr == '\0') {
                    msg.add(intVal);
                  } 
                  // Try to parse as float
                  else {
                    float floatVal = strtof(token, &endptr);
                    if (*endptr == '\0') {
                      msg.add(floatVal);
                    }
                    // Handle boolean (true/false)
                    else if (strcasecmp(token, "true") == 0) {
                      msg.add(true);
                    }
                    else if (strcasecmp(token, "false") == 0) {
                      msg.add(false);
                    }
                    // Default to string
                    else {
                      msg.add(token);
                    }
                  }
                  token = strtok(nullptr, " ");
                }
              }

              Serial.print("[226] Sending OSC to ");
              Serial.print(ipPortPairs[j][0]);
              Serial.print(":");
              Serial.print(ipPortPairs[j][1].toInt());
              Serial.print(" (address: ");
              Serial.print(oscAddress);
              Serial.println(")");

              if (Udp.beginPacket(ipPortPairs[j][0].c_str(), ipPortPairs[j][1].toInt())) {
                msg.send(Udp);
                if (Udp.endPacket()) {
                  Serial.println("[226] OSC packet sent successfully");
                } else {
                  Serial.println("[226] ERROR: Failed to end UDP packet");
                }
              } else {
                Serial.println("[226] ERROR: Failed to begin UDP packet");
              }
            }
          }
          sendByte(128);
          break;
        }

      case 225:
        {
          // ------------------------------------------------------------------------------
          // start byte 225 = Save unified config (OSC, Tasmota, cues)
          // ------------------------------------------------------------------------------
          // OSC settings should already be saved when set via command 229
          // This command can trigger a save of all current configs
          settings.begin("mysettings", false);
          // Save OSC settings
          settings.putString("oscServerIP", oscServerIP);
          settings.putString("oscServerPort", oscServerPort);
          settings.putString("oscServerIP2", oscServerIP2);
          settings.putString("oscServerPort2", oscServerPort2);
          settings.putString("oscServerIP3", oscServerIP3);
          settings.putString("oscServerPort3", oscServerPort3);
          settings.putString("oscServerIP4", oscServerIP4);
          settings.putString("oscServerPort4", oscServerPort4);
          settings.putString("oscServerIP5", oscServerIP5);
          settings.putString("oscServerPort5", oscServerPort5);
          // Save Tasmota device count
          settings.putInt("tasmotaCount", tasmotaDeviceCount);
          settings.end();
          sendByte(128);
          break;
        }

      case 224:
        {
          // ------------------------------------------------------------------------------
          // start byte 224 = Load unified config (on startup)
          // ------------------------------------------------------------------------------
          // Configs are loaded in setup(), this command can trigger a reload
          settings.begin("mysettings", false);
          oscServerIP = settings.getString("oscServerIP", "192.168.178.244");
          oscServerPort = settings.getString("oscServerPort", "8888");
          oscServerIP2 = settings.getString("oscServerIP2", "0.0.0.0");
          oscServerPort2 = settings.getString("oscServerPort2", "0");
          oscServerIP3 = settings.getString("oscServerIP3", "0.0.0.0");
          oscServerPort3 = settings.getString("oscServerPort3", "0");
          oscServerIP4 = settings.getString("oscServerIP4", "0.0.0.0");
          oscServerPort4 = settings.getString("oscServerPort4", "0");
          oscServerIP5 = settings.getString("oscServerIP5", "0.0.0.0");
          oscServerPort5 = settings.getString("oscServerPort5", "0");
          
          tasmotaDeviceCount = settings.getInt("tasmotaCount", 0);
          for (int i = 0; i < tasmotaDeviceCount && i < 10; i++) {
            String prefix = "tasmota" + String(i);
            tasmotaDevices[i].name = settings.getString((prefix + "name").c_str(), "");
            tasmotaDevices[i].ip = settings.getString((prefix + "ip").c_str(), "");
            tasmotaDevices[i].username = settings.getString((prefix + "user").c_str(), "");
            tasmotaDevices[i].password = settings.getString((prefix + "pass").c_str(), "");
          }
          settings.end();
          sendByte(128);
          break;
        }

      case 223:
        {
          // ------------------------------------------------------------------------------
          // start byte 223 = Get Tasmota device list
          // ------------------------------------------------------------------------------
          // Reload from EEPROM to ensure we have the latest data
          settings.begin("mysettings", false);
          tasmotaDeviceCount = settings.getInt("tasmotaCount", 0);
          Serial.print("Loading from EEPROM - tasmotaDeviceCount: ");
          Serial.println(tasmotaDeviceCount);
          for (int i = 0; i < 10; i++) {
            String prefix = "tasmota" + String(i);
            tasmotaDevices[i].name = settings.getString((prefix + "name").c_str(), "");
            tasmotaDevices[i].ip = settings.getString((prefix + "ip").c_str(), "");
            tasmotaDevices[i].username = settings.getString((prefix + "user").c_str(), "");
            tasmotaDevices[i].password = settings.getString((prefix + "pass").c_str(), "");
            Serial.print("Device ");
            Serial.print(i);
            Serial.print(": name='");
            Serial.print(tasmotaDevices[i].name);
            Serial.print("' (");
            Serial.print(tasmotaDevices[i].name.length());
            Serial.print("), ip='");
            Serial.print(tasmotaDevices[i].ip);
            Serial.print("' (");
            Serial.print(tasmotaDevices[i].ip.length());
            Serial.println(")");
          }
          settings.end();
          
          // Format: name1[129]ip1[129]name2[129]ip2[129]...name10[129]ip10[129]
          // Returns exactly 20 fields: name1, ip1, name2, ip2, ..., name10, ip10
          // Always send exactly 20 fields, even if empty
          // For empty slots: two consecutive delimiters represent empty name and empty IP
          String deviceList = "";
          for (int i = 0; i < 10; i++) {
            if (tasmotaDevices[i].name.length() > 0 || tasmotaDevices[i].ip.length() > 0) {
              // Device has data: send name[129]ip (even if one is empty)
              if (i > 0) deviceList += char(129);  // Delimiter before each device (except first)
              deviceList += tasmotaDevices[i].name + char(129) + tasmotaDevices[i].ip;
            } else {
              // Empty slot: send two consecutive delimiters [129][129]
              // This creates: empty name field[129]empty IP field
              if (i > 0) deviceList += char(129);  // Delimiter before each device (except first)
              deviceList += char(129);  // Empty name field delimiter
              deviceList += char(129);  // Empty IP field delimiter (this creates the empty IP field)
            }
          }
          // Debug: print what we're sending
          Serial.print("Sending Tasmota device list (223): ");
          Serial.print(deviceList.length());
          Serial.print(" bytes (should be at least 19 delimiters = ");
          Serial.print((10 * 2 - 1));
          Serial.println(" bytes minimum)");
          Serial.print("DeviceList string: ");
          for (unsigned int i = 0; i < deviceList.length() && i < 50; i++) {
            if (deviceList[i] == char(129)) {
              Serial.print("[129]");
            } else {
              Serial.print(deviceList[i]);
            }
          }
          Serial.println();
          send_String_to_c64(deviceList);
          break;
        }

      case 222:
        {
          // ------------------------------------------------------------------------------
          // start byte 222 = Save Tasmota device list
          // ------------------------------------------------------------------------------
          // Format: name1[129]ip1[129]name2[129]ip2[129]...name10[129]ip10[129]
          // Receives 20 fields (10 devices × 2 fields each)
          receive_buffer_from_C64(1);
          char deviceData[512];
          for (int i = 0; i < inbuffersize && i < 511; i++) {
            deviceData[i] = screenCode_to_Ascii(inbuffer[i]);
          }
          deviceData[inbuffersize] = '\0';

          String data = String(deviceData);
          Serial.print("Received data length: ");
          Serial.println(data.length());
          Serial.print("First 100 chars: ");
          for (int i = 0; i < data.length() && i < 100; i++) {
            if (data[i] == char(129)) {
              Serial.print("[129]");
            } else if (data[i] == '\0') {
              Serial.print("[0]");
            } else {
              Serial.print(data[i]);
            }
          }
          Serial.println();
          
          int lastPos = 0;
          tasmotaDeviceCount = 0;

          // Parse all 20 fields (10 devices × 2 fields: name and IP)
          // Fields are: name1, ip1, name2, ip2, ..., name10, ip10
          // Format: name1[129]ip1[129]name2[129]ip2[129]...name10[129]ip10[129]
          for (int fieldIndex = 0; fieldIndex < 20; fieldIndex++) {
            int pos = data.indexOf(char(129), lastPos);
            if (pos < 0) {
              // No more delimiters, this is the last field
              if (fieldIndex < 19) {
                // Should have more fields, but no delimiter found - treat rest as last field
                pos = data.length();
              } else {
                // Last field (device 10 IP)
                pos = data.length();
              }
            }
            
            String field = "";
            if (pos > lastPos) {
              field = data.substring(lastPos, pos);
              field.trim();
            }
            // If pos == lastPos, field is empty (consecutive delimiters)
            
            int deviceIndex = fieldIndex / 2;  // Which device (0-9)
            bool isNameField = (fieldIndex % 2 == 0);  // true for name, false for IP
            
            if (isNameField) {
              // Name field
              tasmotaDevices[deviceIndex].name = field;
              Serial.print("Field ");
              Serial.print(fieldIndex);
              Serial.print(" (Device ");
              Serial.print(deviceIndex);
              Serial.print(" name): '");
              Serial.print(field);
              Serial.print("' (");
              Serial.print(field.length());
              Serial.println(")");
            } else {
              // IP field
              tasmotaDevices[deviceIndex].ip = field;
              tasmotaDevices[deviceIndex].username = "";  // Not used in new format
              tasmotaDevices[deviceIndex].password = "";  // Not used in new format
              
              Serial.print("Field ");
              Serial.print(fieldIndex);
              Serial.print(" (Device ");
              Serial.print(deviceIndex);
              Serial.print(" IP): '");
              Serial.print(field);
              Serial.print("' (");
              Serial.print(field.length());
              Serial.println(")");
              
              // Update device count if both name and IP are non-empty
              if (tasmotaDevices[deviceIndex].name.length() > 0 && field.length() > 0) {
                tasmotaDeviceCount = deviceIndex + 1;  // Update device count
                Serial.print("Updated tasmotaDeviceCount to: ");
                Serial.println(tasmotaDeviceCount);
              }
            }
            
            lastPos = pos + 1;
            if (pos >= data.length() && fieldIndex < 19) {
              // Ran out of data before getting all 20 fields - fill rest with empty
              for (int remaining = fieldIndex + 1; remaining < 20; remaining++) {
                int remDeviceIndex = remaining / 2;
                bool remIsNameField = (remaining % 2 == 0);
                if (remIsNameField) {
                  tasmotaDevices[remDeviceIndex].name = "";
                } else {
                  tasmotaDevices[remDeviceIndex].ip = "";
                }
              }
              break;
            }
          }

          // Save all devices to EEPROM
          settings.begin("mysettings", false);
          // Always save all 10 slots to EEPROM (even empty ones)
          for (int i = 0; i < 10; i++) {
            String prefix = "tasmota" + String(i);
            settings.putString((prefix + "name").c_str(), tasmotaDevices[i].name);
            settings.putString((prefix + "ip").c_str(), tasmotaDevices[i].ip);
            settings.putString((prefix + "user").c_str(), "");  // Not used
            settings.putString((prefix + "pass").c_str(), "");  // Not used
          }
          settings.putInt("tasmotaCount", tasmotaDeviceCount);
          settings.end();
          
          // Force EEPROM commit to ensure data is saved
          settings.begin("mysettings", false);
          settings.end();

          // Return the saved data to C64 (same format as command 223)
          // Format: name1[129]ip1[129]name2[129]ip2[129]...name10[129]ip10[129]
          // Returns exactly 20 fields: name1, ip1, name2, ip2, ..., name10, ip10
          // For empty slots: two consecutive delimiters represent empty name and empty IP
          String deviceList = "";
          for (int i = 0; i < 10; i++) {
            // Always send data for all devices, regardless of tasmotaDeviceCount
            // This ensures all 20 fields are always returned
            if (tasmotaDevices[i].name.length() > 0 || tasmotaDevices[i].ip.length() > 0) {
              // Device has at least one field with data: send name[129]ip
              if (i > 0) deviceList += char(129);  // Delimiter before each device (except first)
              deviceList += tasmotaDevices[i].name + char(129) + tasmotaDevices[i].ip;
            } else {
              // Empty slot: send two consecutive delimiters [129][129]
              // This creates: empty name field[129]empty IP field
              if (i > 0) deviceList += char(129);  // Delimiter before each device (except first)
              deviceList += char(129);  // Empty name field delimiter
              deviceList += char(129);  // Empty IP field delimiter (this creates the empty IP field)
            }
          }
          Serial.print("Returning saved data (222): ");
          Serial.print(deviceList.length());
          Serial.println(" bytes");
          send_String_to_c64(deviceList);
          break;
        }

      case 221:
        {
          // ------------------------------------------------------------------------------
          // start byte 221 = Save cue list to ESP32
          // ------------------------------------------------------------------------------
          receive_buffer_from_C64(2);  // Cue list data
          // Store cue list data (raw screen codes)
          settings.begin("mysettings", false);
          // Store as binary data - convert to base64 or store raw
          String cueData = "";
          for (int i = 0; i < inbuffersize && i < 2340; i++) {
            cueData += char(inbuffer[i]);
          }
          settings.putString("cueList", cueData);
          settings.end();
          sendByte(128);
          break;
        }

      case 220:
        {
          // ------------------------------------------------------------------------------
          // start byte 220 = Load cue list from ESP32
          // ------------------------------------------------------------------------------
          settings.begin("mysettings", false);
          String cueData = settings.getString("cueList", "");
          settings.end();
          
          if (cueData.length() > 0) {
            // Send cue list data back to C64 (as screen codes)
            for (unsigned int i = 0; i < cueData.length(); i++) {
              sendByte(Ascii_to_screenCode(cueData[i]));
            }
          }
          sendByte(128);
          break;
        }

      case 228:
        {
          // ------------------------------------------------------------------------------
          // start byte 228 = Set OSC server IP and port
          // ------------------------------------------------------------------------------
          receive_buffer_from_C64(1);  // Get all IP + port pairs in one buffer

          // Convert to ASCII and null-terminate
          char oscMsg[256];
          int i;
          for (i = 0; i < inbuffersize && i < 255; i++) {
            oscMsg[i] = screenCode_to_Ascii(inbuffer[i]);
            if (oscMsg[i] == '\0') break;
          }
          oscMsg[i] = '\0';

          // Split into 10 fields: ip1[129]port1[129]ip2[129]port2[129]...
          // Using char(129) as delimiter for all fields
          String fields[10];
          int fieldIndex = 0;
          int lastPos = 0;
          
          for (int i = 0; i <= inbuffersize && fieldIndex < 10; i++) {
            if (i == inbuffersize || oscMsg[i] == '\x81') {  // 129 in hex
              oscMsg[i] = '\0';
              if (i > lastPos) {
                fields[fieldIndex] = String(&oscMsg[lastPos]);
                fields[fieldIndex].trim();
                fieldIndex++;
              }
              lastPos = i + 1;
            }
          }
          
          // Assign fields to OSC IP/port pairs
          if (fieldIndex >= 2) {
            oscServerIP = fields[0];
            oscServerPort = fields[1];
            // Remove non-digit characters from port
            for (int k = 0; k < oscServerPort.length(); k++) {
              if (!isdigit(oscServerPort[k])) {
                oscServerPort.remove(k);
                k--;
              }
            }
          }
          if (fieldIndex >= 4) {
            oscServerIP2 = fields[2];
            oscServerPort2 = fields[3];
            for (int k = 0; k < oscServerPort2.length(); k++) {
              if (!isdigit(oscServerPort2[k])) {
                oscServerPort2.remove(k);
                k--;
              }
            }
          }
          if (fieldIndex >= 6) {
            oscServerIP3 = fields[4];
            oscServerPort3 = fields[5];
            for (int k = 0; k < oscServerPort3.length(); k++) {
              if (!isdigit(oscServerPort3[k])) {
                oscServerPort3.remove(k);
                k--;
              }
            }
          }
          if (fieldIndex >= 8) {
            oscServerIP4 = fields[6];
            oscServerPort4 = fields[7];
            for (int k = 0; k < oscServerPort4.length(); k++) {
              if (!isdigit(oscServerPort4[k])) {
                oscServerPort4.remove(k);
                k--;
              }
            }
          }
          if (fieldIndex >= 10) {
            oscServerIP5 = fields[8];
            oscServerPort5 = fields[9];
            for (int k = 0; k < oscServerPort5.length(); k++) {
              if (!isdigit(oscServerPort5[k])) {
                oscServerPort5.remove(k);
                k--;
              }
            }
          }

          // Save all pairs to EEPROM
          settings.begin("mysettings", false);
          settings.putString("oscServerIP", oscServerIP);
          settings.putString("oscServerPort", oscServerPort);
          settings.putString("oscServerIP2", oscServerIP2);
          settings.putString("oscServerPort2", oscServerPort2);
          settings.putString("oscServerIP3", oscServerIP3);
          settings.putString("oscServerPort3", oscServerPort3);
          settings.putString("oscServerIP4", oscServerIP4);
          settings.putString("oscServerPort4", oscServerPort4);
          settings.putString("oscServerIP5", oscServerIP5);
          settings.putString("oscServerPort5", oscServerPort5);
          settings.end();

          sendByte(128);
          break;
        }

      default:
        {
          sendByte(128);
          break;
        }
    }  // end of case statements
  }    // end of "if (dataFromC64)"
}  // end of main loop

// ******************************************************************************
// void to set a byte in the 74ls244 buffer
// ******************************************************************************
void outByte(byte c) {
  digitalWrite(oC64D0, bool(c & B00000001));
  digitalWrite(oC64D1, bool(c & B00000010));
  digitalWrite(oC64D2, bool(c & B00000100));
  digitalWrite(oC64D3, bool(c & B00001000));
  digitalWrite(oC64D4, bool(c & B00010000));
  digitalWrite(oC64D5, bool(c & B00100000));
  digitalWrite(oC64D6, bool(c & B01000000));
  digitalWrite(oC64D7, bool(c & B10000000));
}

// ******************************************************************************
// void: send a string to the C64
// ******************************************************************************
void send_String_to_c64(String s) {
  s.toCharArray(msgbuffer, s.length() + 1);       // place the ssid in the output buffer
  send_buffer_to_C64(msgbuffer, s.length() + 1);  // and send the buffer
}

// ******************************************************************************
//  receive characters from the C64 and store them in a buffer
//  should always return false
//  returns True if an rx error was encountered
// ******************************************************************************
bool receive_buffer_from_C64(int cnt) {
  // cnt is the number of transmissions we put into this buffer
  // This number is 1 most of the time
  // but in the configuration screens the C64 will send multiple items at once (like ssid and password)

  // Note: receivingBuffer flag should be set BEFORE calling this function to prevent main loop from processing bytes
  int i = 0;
  bool rxError = false;
  while (cnt > 0) {
    digitalWrite(oC64D7, HIGH);  // ready for next byte
    unsigned long timeOut = millis() + 500;
    while (dataFromC64 == false) {
      delayMicroseconds(2);  // wait for next byte
      if (millis() > timeOut) {
        ch = 128;
        dataFromC64 = true;
        rxError = true;
#ifdef debug
        Serial.println("Timeout in receive buffer");
#endif
      }
    }
    digitalWrite(oC64D7, LOW);
    dataFromC64 = false;
    inbuffer[i] = ch;
    i++;
    if (i > 248) {  //this should never happen
#ifdef debug
      Serial.print("Error: inbuffer is about to flow over!");
#endif
      ch = 128;
      cnt = 0;
      break;
    }
    if (ch == 128) {
      cnt--;
      inbuffer[i] = 129;
      i++;
    }
  }
  i--;
  inbuffer[i] = 0;  // close the buffer
  inbuffersize = i;
  receivingBuffer = false;  // Clear flag - buffer reception complete
  return rxError;
}

// ******************************************************************************
// Send the content of the outbuffer to the C64
// ******************************************************************************
void send_buffer_to_C64(char *buffer, int buffSize) {
  // send the content of a buffer to the C64
  for (int x = 0; x < buffSize - 1; x++) sendByte(Ascii_to_screenCode(buffer[x]));
  sendByte(128);  // all done, send end byte
}

// ******************************************************************************
// pull the NMI line low for a few microseconds
// ******************************************************************************
void triggerNMI() {
#ifdef Mega65
  delayMicroseconds(10);  // extra delay for MEGA65.. this is a test
  digitalWrite(oC64NMI, !invert_nmi_signal);
  delayMicroseconds(175);  // minimal 100 microseconds delay
  delayMicroseconds(75);   // extra delay for MEGA65.. this is a test
  digitalWrite(oC64NMI, invert_nmi_signal);
#else
  digitalWrite(oC64NMI, !invert_nmi_signal);
  delayMicroseconds(175);  // minimal 100 microseconds delay
  digitalWrite(oC64NMI, invert_nmi_signal);
#endif
}


// ******************************************************************************
// send a single byte to the C64
// Should always return false
// returns true is there was an error.
// ******************************************************************************
bool sendByte(byte b) {
  bool txError = 0;
  outByte(b);
  io2 = false;
  triggerNMI();
  unsigned long timeOut = millis() + 300;
  while ((io2 == false) and !txError) {  // wait for io2 interupt
    delayMicroseconds(2);
    if (millis() > timeOut) txError = 1;
  }
#ifdef debug
  if (millis() > timeOut) Serial.println("Timeout in sendByte");
#endif
  io2 = false;
  return txError;
}

// ******************************************************************************
// Send out urgent message if available (error messages)
// ******************************************************************************
void doUrgentMessage() {
  if (urgentMessage != "") {
    int color = 146;
    if (urgentMessage.startsWith("[grn]")) color = 149;
    urgentMessage.remove(0, 5);
    sendByte(1);
    sendByte(143);
    sendByte(color);
    send_String_to_c64(urgentMessage);
    urgentMessage = "";
  }
}

String urlEncode(const String &s)
{
    String out;
    char hex[4];

    for (size_t i = 0; i < s.length(); i++) {
        char c = s[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out += c;
        } else {
            sprintf(hex, "%%%02X", (unsigned char)c);
            out += hex;
        }
    }
    return out;
}

// ******************************************************************************
// Send Tasmota HTTP command
// ******************************************************************************
void sendTasmotaCommand(String deviceIP, String command, String username, String password) {
  if (!isWifiCoreConnected) {
    Serial.println("[Tasmota] WiFi not connected, skipping command");
    return;
  }
  
  String url = "http://" + deviceIP + "/cm?cmnd=" + urlEncode(command);
  
  Serial.print("[Tasmota] Sending to: ");
  Serial.println(url);
  
  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);
  
  // Add authentication if provided
  if (username.length() > 0 && password.length() > 0) {
    String auth = username + ":" + password;
    http.setAuthorization(auth.c_str());
  }
  
  int httpResponseCode = http.GET();
  
  if (httpResponseCode > 0) {
    Serial.print("[Tasmota] Success: ");
    Serial.print(url);
    Serial.print(" -> HTTP ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("[Tasmota] Failed: ");
    Serial.print(url);
    Serial.print(" -> Error code ");
    Serial.println(httpResponseCode);
  }
  
  http.end();
  client.stop();
}