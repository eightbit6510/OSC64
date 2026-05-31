# ESP32 Command Reference

Simple list of all command cases in `C64_Chat_3_85.ino` and what they do:

## High Number Commands (254-250)
- **254** - C64 triggers call to website for new public message
- **253** - New chat message from C64 to database
- **252** - C64 sends new WiFi network name (SSID), password, and time offset
- **251** - C64 asks for current WiFi SSID, password, and time offset
- **250** - C64 asks for first full page of messages (during startup)

## System & Status Commands (249-245)
- **249** - Get result of last send action (253)
- **248** - C64 asks for WiFi connection status
- **247** - C64 triggers call to website for new private message
- **246** - Set chat server IP/FQDN
- **245** - Check if ESP is connected at all, or running in simulation mode

## Configuration Commands (244-235)
- **244** - Reset to factory defaults
- **243** - C64 asks for MAC address, registration ID, nickname, and registration status
- **242** - Do firmware update
- **241** - Get number of unread private messages
- **240** - C64 sends new registration ID and nickname to ESP32
- **239** - C64 asks if updated firmware is available
- **238** - C64 triggers call to chat server to test connectivity
- **237** - Get chat server connectivity status
- **236** - C64 asks for server configuration status and server name
- **235** - C64 sends server configuration status

## User List Commands (234-233)
- **234** - Get user list first page
- **233** - Get user list next page

## Unused Commands
- **232** - (Unused)
- **231** - (Unused)
- **230** - (Unused)

## OSC & Tasmota Commands (229-220)
- **229** - Scroll up or down (message scrolling)
- **228** - Set OSC server IP and port (up to 5 pairs)
- **227** - Get OSC server IP and port
- **226** - Send OSC message (or Tasmota command if prefixed with "tasmota:" or "tas:")
- **225** - Save unified config (OSC, Tasmota, cues)
- **224** - Load unified config
- **223** - Get Tasmota device list
- **222** - Save Tasmota device list
- **221** - Save cue list to ESP32
- **220** - Load cue list from ESP32

## Notes:
- All commands receive data via `receive_buffer_from_C64(N)` where N is the number of transmission buffers
- Commands ending with 128 (byte 128) indicate completion
- Delimiter character 129 (0x81) is used to separate fields in multi-field data
- Screen codes are converted to ASCII using `screenCode_to_Ascii()` and vice versa with `Ascii_to_screenCode()`
