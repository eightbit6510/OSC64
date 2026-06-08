#include "utils.h"

// ******************************************************************************
// translate screen codes to ascii
// ******************************************************************************
char screenCode_to_Ascii(byte screenCode) {

  byte screentoascii[] = { 64, 97, 98, 99, 100, 101, 102, 103, 104, 105,
                           106, 107, 108, 109, 110, 111, 112, 113, 114, 115,
                           116, 117, 118, 119, 120, 121, 122, 91, 92, 93,
                           94, 95, 32, 33, 34, 125, 36, 37, 38, 39,
                           40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
                           50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
                           60, 61, 62, 63, 95, 65, 66, 67, 68, 69,
                           70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
                           80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
                           90, 43, 32, 124, 32, 32, 32, 32, 32, 32,
                           95, 32, 32, 32, 32, 32, 32, 32, 32, 32,
                           32, 95, 32, 32, 32, 32, 32, 32, 32, 32,
                           32, 32, 32, 32, 32, 32, 32, 32, 32 };

  char result = char(screenCode);
  if (screenCode < 129) result = char(screentoascii[screenCode]);
  return result;
}


// ******************************************************************************
// translate ascii to c64 screen codes
// ******************************************************************************
byte Ascii_to_screenCode(char ascii) {

  byte asciitoscreen[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
                           11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
                           22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
                           33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
                           44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54,
                           55, 56, 57, 58, 59, 60, 61, 62, 63, 0, 65,
                           66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
                           77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87,
                           88, 89, 90, 27, 92, 29, 30, 100, 39, 1, 2,
                           3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13,
                           14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24,
                           25, 26, 27, 93, 35, 64, 32, 32 };
  byte result = ascii;
  if (int(ascii) < 129) result = byte(asciitoscreen[int(ascii)]);
  return result;
}

byte checksum(byte data[], int datasize) {
  byte sum = 0;
  for (int i = 0; i < datasize; i++) {
    sum += data[i];
  }
  return -sum;
}

int x2i(char* s) {
  int x = 0;
  for (;;) {
    char c = *s;
    if (c >= '0' && c <= '9') {
      x *= 16;
      x += c - '0';
    } else if (c >= 'a' && c <= 'f') {
      x *= 16;
      x += (c - 'a') + 10;
    } else break;
    s++;
  }
  return x;
}

String getValue(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void loadPrgfile() {
  bool nmiConfirmed = false;
  int startaddress = (prgfile[1] * 0x100) + prgfile[0];  // get the start address from the prg file ($0801)
  int endaddress = startaddress + sizeof(prgfile) - 2;   // calculate the end address
  delay(2000);                                           // give the C64 some time to boot
  Serial.println("C64 Version.");
  Serial.println("Wait for the start signal");
  while (ch != 100) {
    delay(1);  // wait for the c64 to send byte 100
  }
  delay(10);
  Serial.println("------ LOAD PRG FILE ------");
  sendByte(20);                                // send the border color during loading 0-15, default = 6, 20 is blink (loading bars)
  sendByte(0);                                 // send the screen color during loading 0-15, default = 6, 20 is blink (loading bars)
  sendByte(prgfile[0]);                        // send the start address (low byte = 01)
  sendByte(prgfile[1]);                        // send the start address (high byte = 08)
  sendByte(lowByte(endaddress));               // send the END address (low byte)
  sendByte(highByte(endaddress));              // send the END address (high byte)
  for (int x = 2; x < sizeof(prgfile); x++) {  // Now send all the rest of the bytes
    if (sendByte(prgfile[x])) {
      Serial.println("Loading failed, Reboot and try again");
      delay(100);
      ESP.restart();
      break;
    };
  }
  sendByte(0);
  sendByte(0);
  Serial.println("------ PRG FILE DONE ------");
}
