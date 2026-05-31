#include <Preferences.h>

Preferences settings;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("Clearing EEPROM (NVS)...");
  
  // Clear the "mysettings" namespace
  settings.begin("mysettings", false);
  settings.clear();
  settings.end();
  
  Serial.println("EEPROM cleared successfully!");
  Serial.println("You can now upload your main sketch.");
  
  // Keep the LED on to indicate completion
  pinMode(2, OUTPUT);  // Built-in LED pin (adjust if needed)
  digitalWrite(2, HIGH);
}

void loop() {
  // Do nothing
  delay(1000);
}
