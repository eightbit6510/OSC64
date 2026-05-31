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

// ************************************************************************************
// BASE64 encode / decode functions.
// based on https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
// ************************************************************************************
String base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

String my_base64_encode(char* buf, int bufLen) {
  String ret;
  int i = 0;
  int j = 0;

  unsigned char char_array_4[4], char_array_3[3];
  while (bufLen--) {
    char_array_3[i++] = *(buf++);

    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for (i = 0; (i < 4); i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while ((i++ < 3))
      ret += '=';
  }
  return ret;
}

String my_base64_decode(String const& encoded_string) {
  int inlen = encoded_string.length();
  int i = 0;
  int j = 0;
  int k = 0;
  unsigned char char_array_4[4], char_array_3[3];
  String ret;

  while (inlen-- && (encoded_string[k] != '=') && is_base64(encoded_string[k])) {
    char_array_4[i++] = encoded_string[k];
    k++;
    if (i == 4) {
      for (i = 0; i < 4; i++) {
        char_array_4[i] = (char)base64_chars.indexOf(char_array_4[i]);
      }

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
      for (i = 0; (i < 3); i++) {
        ret += (char)char_array_3[i];
      }
      i = 0;
    }
  }

  if (i) {
    for (j = 0; j < i; j++) {
      char_array_4[j] = (char)base64_chars.indexOf(char_array_4[j]);
    }

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    for (j = 0; (j < i - 1); j++) {
      ret += (char)char_array_3[j];
    }
  }
  return ret;
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


bool getMessageFromMMBuffer(char *sourceBuffer, int *bufferIndex, bool isPrivate) {
  // do we have any messages in the page buffer?
  // find the first '{' in the page buffer
  int p = 0;
  char cc = 0;
  bool found = false;
  while (cc != '{' and p++ < 10) {  // find first {
    cc = sourceBuffer[(*bufferIndex)++];
  }
  if (cc == '{') {  // copy to message buffer until we find '}'
    msgbuffer[0] = cc;
    found = true;
    getMessage = false;
    p = 1;
    while (cc != '}') {
      cc = sourceBuffer[(*bufferIndex)++];
      if (cc != 10) msgbuffer[p++] = cc;  // put this line into the msgbuffer buffer
    }
  }
  if (!found) {
    for (int y = 0; y < 3500; y++) sourceBuffer[y] = 0;  // clear the buffer
    *bufferIndex = 0;    
  }

  return found;
}

// ******************************************************************************
// Deserialize the json encoded messages
// ******************************************************************************
int Deserialize() {
  int haveMessage = 0;
  msgbuffersize = 0;
  DynamicJsonDocument doc(512);                                  // next we want to analyse the json data
  DeserializationError error = deserializeJson(doc, msgbuffer);  // deserialize the json document
  if (!error) {
    unsigned long newMessageId = doc["rowid"];
    // if we get a new message id back from the database, that means we have a new message
    // if the database returns the same message id, there is no new message for us..
    String channel = doc["channel"];
    pmCount = doc["pm"];

    if ((channel == "public") and (newMessageId != messageIds[0])) {
      tempMessageIds[0] = newMessageId;
      haveMessage = 1;
    }

    if ((channel == "private") and (newMessageId != messageIds[1])) {
      tempMessageIds[1] = newMessageId;
      String nickname = doc["nickname"];
      haveMessage = 2;
    }
    
    if ((channel == "scroll") and (newMessageId != topMes)){
      tempMessageIds[2] = newMessageId;
      haveMessage = 3;
    }
    
    if (haveMessage != 0) {
      String message = doc["message"];
      int lines = doc["lines"];
      String decoded_message = char(lines) + my_base64_decode(message);
      decoded_message.toCharArray(msgbuffer, decoded_message.length() + 1);
      msgbuffersize = (int)decoded_message.length() + 1;                  
    } 
    
    doc.clear();
  } // else {error is deserialize}

  return haveMessage;
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
