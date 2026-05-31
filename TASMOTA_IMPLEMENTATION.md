# Tasmota Integration Implementation Plan

## Overview
This document outlines the implementation of Tasmota device control integration into the OSC64 system using Option C (Hybrid approach).

## ESP32 Side (Completed)

### New Features Added:
1. **Tasmota Device Storage**
   - Structure: `TasmotaDevice` with name, IP, username, password
   - Max 10 devices stored in EEPROM
   - Loaded on startup

2. **HTTP Request Handler**
   - Function: `sendTasmotaCommand(deviceIP, command, username, password)`
   - Sends HTTP GET to `http://<IP>/cm?cmnd=<COMMAND>`
   - Supports authentication

3. **OSC Handler Enhancement**
   - Detects `tasmota:` or `tas:` prefix in OSC messages
   - Resolves device name to IP address
   - Format: `tasmota:name command` or `tas:name command`

4. **Unified Configuration Storage**
   - Command 225: Save all configs (OSC, Tasmota, cues)
   - Command 224: Load all configs
   - Command 223: Get Tasmota device list
   - Command 222: Save single Tasmota device
   - Command 221: Save cue list
   - Command 220: Load cue list

## C64 Side (To Be Implemented)

### Menu Changes:
- **F8**: Changed from "Output Setup" to "Tasmota Setup"

### New Screens Needed:

1. **Tasmota Setup Screen** (F8)
   - List configured devices (up to 10)
   - Navigation: Arrow keys to select device
   - F1: Add new device
   - F3: Edit selected device
   - F5: Delete selected device
   - F7: Exit to main menu
   - F1 in device edit: Save device

2. **Device Edit Screen**
   - Fields:
     - Device Name (max 20 chars)
     - IP Address (max 15 chars)
     - Username (optional, max 20 chars)
     - Password (optional, max 20 chars)
   - Format sent to ESP32: `name[129]ip[129]username[129]password[128]`

3. **Cue List Save/Restore**
   - Add to OSC Setup screen:
     - F2: Save cue list to EEPROM
     - F4: Load cue list from EEPROM
   - On startup: Load cue list if available

### Command Byte Usage:
- 220: Load cue list from ESP32
- 221: Save cue list to ESP32
- 222: Save Tasmota device to ESP32
- 223: Get Tasmota device list from ESP32
- 224: Load unified config (on startup)
- 225: Save unified config

### OSC Cue Format:
Users can add Tasmota commands to OSC cues:
- Format: `tasmota:devicename Power On`
- Format: `tasmota:devicename Dimmer 50`
- Format: `tas:devicename Power Toggle`

The ESP32 will detect the prefix and convert to HTTP request.

## Memory Layout

### Cue List Storage:
- Location: `$6800` (26 cues × 90 bytes = 2340 bytes)
- Each cue: 80 chars + 10 bytes metadata
- Saved as raw screen codes to ESP32 EEPROM

### Tasmota Device Storage (ESP32):
- Key format: `tasmota0name`, `tasmota0ip`, `tasmota0user`, `tasmota0pass`
- Device count stored in `tasmotaCount`

## Testing Checklist:
- [ ] Add Tasmota device via F8 menu
- [ ] Edit existing device
- [ ] Delete device
- [ ] Save unified config
- [ ] Load unified config on reboot
- [ ] Save cue list
- [ ] Load cue list on reboot
- [ ] Send Tasmota command via OSC cue
- [ ] Test with real Tasmota device
