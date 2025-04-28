When using an 8K EEPROM, such as the Atmel 28C64, use the 8K file. The bootloader will reside at address 0x0000 in the EEPROM.
For a 32K EEPROM, like the Winbond W27E257, use the 32K file. In this case, the bootloader is located at address 0x6000.
When using a 64K EEPROM, such as the Winbond W27C512, use the 64K file. Here, the bootloader resides at address 0xE000.
