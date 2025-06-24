# OSC64

**The OSC addition for your Chat64 Cartridge on the Commodore64.**

Information about this great cartridge by Bart Venneker and Theo van den Beld here: https://chat64.nl/
![DevBoard](/Artwork/devrev28.png)

All original files are forked 1:1 in this repo, I just added extra osc64 folders/files in the ESP32 Sketch folder and the Kick ASM folder.

This is an altered version of the original chat64 software written by Bart and Theo.
It adds the ability to send OSC ( https://en.wikipedia.org/wiki/Open_Sound_Control )to 5 different devices while still maintaining the original chat functionality.
Some menu options have been altered and the original help and about screens are replaced to make place for the osc setup and send screens. The assembly code might look a bit hacky because I had some trouble implementing my ideas in the existing code ;)

The OSC commands can be typed on the OSC send screen. The screen cycles to the next OSC command after pressing [RETURN]
The osc parser accepts string, integer, float and boolean statements.
In the OSC setup screen entering ip 0.0.0.0 and port 0 skips the ip.

Some features I'd like to add in the future:

- Restructure menu 
- Improved cue list with:
                          -  Trigger at [Time of Day]
                          -  Auto-follow [Wait Time] to Cue [#]
- Paddle support to send realtime integer data along with a pre configured OSC command
- OSC busking screen that allows for binding OSC commands or trigger cues to different keys of the keyboard.
- MQTT ( https://en.wikipedia.org/wiki/MQTT ) protocol for controlling internet of things devices 
