# OSC64

**The OSC addition for your Chat64 Cartridge on the Commodore64.**

Information about this great cartridge by Bart Venneker and Theo van den Beld here: https://chat64.nl/
![DevBoard](/Artwork/devrev28.png)

All original files are forked 1:1 in this repo, I just added extra osc64 folders/files in the ESP32 Sketch folder and the Kick ASM folder.

This is an altered version of the original chat64 software written by Bart and Theo.
It adds the ability to send OSC ( https://en.wikipedia.org/wiki/Open_Sound_Control )to 5 different devices while still maintaining the original chat functionality.
Some menu options have been altered and the original help and about screens are replaced to make place for the osc setup and send screens. The assembly code might look a bit hacky because I had some trouble implementing my ideas in the existing code ;)

The osc parser accepts string, integer, float and boolean statements.
In the OSC setup screen entering ip 0.0.0.0 and port 0 skips the ip.

*** New Version info: ***

- Improved OSC send screen. F3 toggles Playback mode and Edit mode screens. New OSC commands can be max 80 characters long.
![Cuelist](/Artwork/Screenshot 2025-07-03 at 00.47.18.png)

- F1 in the OSC send screen enters OSC busk mode. Keys A through Z + space and 1 through 0 immediately send an OSC command to the configured IP's

Some features I'd like to add in the future:

- Restructure menu 
- Improve cue list with:
                          -  Trigger at [Time of Day]
                          -  Auto-follow [Wait Time] to Cue [#]
- Paddle support to send realtime integer data along with a pre configured OSC command
- MQTT ( https://en.wikipedia.org/wiki/MQTT ) protocol for controlling internet of things devices 
