#ifndef UTIL_H__
#define UTIL_H__

#include "common.h"

char screenCode_to_Ascii(byte screenCode);
byte Ascii_to_screenCode(char ascii);

byte checksum(byte data[], int datasize);
int x2i(char* s);
String getValue(String data, char separator, int index);
void loadPrgfile();
#endif // UTIL_H__
