// General info
// Relaunch 64 IDE: https://github.com/sjPlot/Relaunch64/releases
// Kick assembler for java 8: https://csdb.dk/release/?id=180813
// Memory Map: http://www.bartvenneker.nl/C64/Mem_map.html
// PETSCI KEY CODES: http://www.bartvenneker.nl/C64/Petscii_codes.html
// SCREEN functions: http://www.bartvenneker.nl/C64/Screen_codes.html
// Kernal Screen functions: http://www.bartvenneker.nl/C64/kernal_screen.html
// All Kernal functions: http://www.bartvenneker.nl/C64/kernal.html
// Petscii editor: https://petscii.krissz.hu/
// Vice emulator: https://vice-emu.sourceforge.io/windows.html

// constants
.const _LINE_COLOR_ = $9c
.const _LINE_POS_ = $fb
.const _BUFFER_POINTER_ = $fb
.const _FIELD_ = $fb
.const _NOTE_ = $fb
.const _SCREENLINE_ = $f7
.const _SCREENCOLM_ = $f8
.const _SCREENADRR_ = $43
.const _TEMPADRR_   = $fe
.const _IO1_ = $de00
.const _IO2_ = $df00
.const outputline = $0590
      

//=========================================================================================================
//   ASM Dialect: Kick assembler by Mads Nielsen
//=========================================================================================================
*=$0800 "BASIC Start"                             // location to put a 1 line basic program so we can just
        .byte $00                                 // first byte of basic memory should be a zero
        .byte $0E, $08                            // Forward address to next basic line
        .byte $0A, $00                            // this will be line 10 ($0A)
        .byte $9E                                 // basic token for SYS
        .text " (2064)"                           // 10 SYS (2064)
        .byte $00, $00, $00                       // end of basic program (addr $080E from above)

*=$810 "Main Program"

//=========================================================================================================
//    Warm start procedure starts at $810
//========================================================================================================= 
main_init:                                        //                                    
    jsr $E544                                     // Clear screen
//    lda #$00                                     // Read control register to latch TOD
//    sta $dc08
    lda #2                                        //
    sta _LINE_COLOR_                              // default line color (green)
    lda #0                                        // 
    sta $d020                                     // Set black screen
    sta $d021                                     // Set black border
    sta CONFIG_STATUS                             //
    sta MUTE                                      // Do not mute the sound
    sta INCONFIG                                  //
    lda #144                                      // Load petscii code for Black Cursor
    jsr $ffd2                                     // output black cursor to the screen
    ldx #24                                       // zero SID sound register (1)
    lda #170                                      //
    sta TIMEOUTVALUE                              //
    lda #0                                        //
                                                  //
!clear_sid_loop:                                  // Clear the SID registers
    sta $d400, x                                  // 
    dex                                           // 
    bne !clear_sid_loop-                          // 
    lda #$80                                      // disable SHIFT-Commodore
    sta $0291                                     //
    lda #<(nmi)                                   // \
    sta $0318                                     //  \ Load our new nmi vector
    lda #>(nmi)                                   //  / And replace the old vector to our own nmi routine
    sta $0319                                     // /        
    jsr !are_we_in_the_matrix+                    // check if we are running inside a simulator (esp32 is disconnected)
    jsr !start_screen+                            // Call the start screen sub routine first
    jsr $E544                                     // Clear screen after start screen
    lda #23                                       // Load 23 into accumulator and use it to
    sta $D018                                     // Switch to LOWER CASE
                                                  //
!check_config_status:                             //
    lda CONFIG_STATUS                             // e(5),c(3),d(4)
    cmp #4                                        // a valid value for CONFIG_STATUS = 5,4,3
    beq !cfg_ok+                                  //
    cmp #3                                        //
    beq !cfg_ok+                                  //
    cmp #5                                        //
    beq !cfg_ok+                                  //
    jsr !delay100+                                //
    jmp !redoMatrixCheck+                         //
!cfg_ok:
    jmp !goto_mainmenu_tramp+                      //
!goto_mainmenu_tramp:
    jmp !goto_mainmenu+
!redoMatrixCheck:                                 // 
    jsr !are_we_in_the_matrix+                    //
    jsr !callstatus+                              //
    jmp !check_config_status-                     //
                                                  //
//=========================================================================================================
//  Vice Simulation check
//=========================================================================================================
!are_we_in_the_matrix:                            // 
    jsr !delay100+                                // this is to check if a real cartridge is attached
    jsr !wait_for_ready_to_receive+               // or if we are running in the Vice simulator
    lda #245                                      // Load number #245 (to check if the esp32 is connected)
    sta _IO1_                                     // write the byte to IO1
                                                  // 
                                                  // Send the ROM version to the cartrdige
!:  ldx #1                                        // x will be our index when we loop over the version text, we start at 1 to skip the first color byte
!sendversion:                                     // 
    jsr !wait_for_ready_to_receive+               // wait for ready to receive (bit D7 goes high)
    lda version,x                                 // load a byte from the version text with index x
    sta _IO1_                                     // send it to IO1
    cmp #128                                      // if the last byte was 128, the buffer is finished
    beq !+                                        // exit in that case
    inx                                           // increase the x index
    jmp !sendversion-                             // jump back to send the next byte
                                                  // 
!:  jsr !delay100+                                // Delay 100... hamsters
    lda _IO2_                                     // read from IO2.
    cmp #128                                      // 
                                                  // 
    beq !exit+                                    // if vice mode, we do not try to communicate with the
    lda #1                                        // cartridge because it will result in error
    sta VICEMODE                                  //  
                                                  //
!exit:                                            // 
    jsr !delay100+                                // Delay 100... hamsters
    rts                                           // 

//=========================================================================================================
//    HIDE AND SHOW CURSOR ROUTINES
//=========================================================================================================
!hide_cursor:
    lda #$01; sta $cc                             // Hide the cursor
    rts                                           //
 
!show_cursor:  
    lda #$00; sta $cc                             // show the cursor
    rts                                           //

//=========================================================================================================
//     Play note and play song
//=========================================================================================================
!playnote:                                        // A=frequency, DELAY = tone length
    cmp #0                                        // if A=0 then play no sound, just a silent delay
    beq !tonelength+                              //
    sta $d401                                     // frequency voice 1 high byte 
    sta $d408                                     //
    lda #7                                        //
    sta $d403                                     //
    lda #7                                        //
    sta $d40a                                     //
    lda #255                                      //
    sta $d402                                     //
    sta $d409                                     //
    lda #0                                        // 
    sta $d400                                     // frequency voice 1 low byte  
    sta $d407                                     //
    lda #143                                      // set volume to max for all voices and MUTE voice 3
    sta $d418                                     //                                                                                             
    lda #0                                        // 
    sta $d405                                     // 
    sta $d40c                                     //
    lda #240                                      //
    sta $d406                                     // 
    sta $d40d                                     //
    lda #33                                       //
    sta $d404                                     //
    lda #65                                       //
    sta $d40b                                     // set triangle wave form (bit 4) and gate bit 0 to 1 (sound on)                                              // 
!tonelength:                                      //
    tya                                           //
    pha                                           // save value in accumulator
    jsr !delay+                                   //    
    pla                                           // restore value in accumulator
    tay                                           //   
    lda #32                                       // stop playing sound on voice 1
    sta $d404                                     //
    sta $d404                                     // 
    lda #64                                       // stop playing sound on voice 2
    sta $d40b                                     //
    sta $d40b                                     //
    lda #128                                      // set volume to NUL for all voices and MUTE voice 3
    sta $d418                                     //                                                                                             
    rts                                           //
                                                  //
!playsongk:                                       //
   lda MUTE 
   cmp #0
   beq !+
   rts
!: ldy #0                                         //
!fetchnote:                                       //
   lda (_NOTE_),y                                 //
   cmp #255                                       //
   beq !exit+                                     //
   sta DELAY                                      //
   iny                                            //
   lda (_NOTE_),y                                 //
   jsr !playnote-                                 //   
   iny                                            //
   jmp !fetchnote-                                //
!exit:                                            //
   rts                                            //
                                                  //
!sounderror:                                      // 
    lda MUTE                                      // if Mute is not zero
    cmp #0                                        // exit
    beq !+                                        //
    rts                                           //
!:  lda #143                                      // set volume to max and mute voice 3
    sta $d418                                     // and store it here
    lda #5                                        // set the pitch very low
    sta $d401                                     // and store it here (voice 1)
    lda #15                                       // 
    sta $d405                                     // set attack / decay lenght (voice 1)
    lda #0                                        // 
    sta $d406                                     // set volume sustain to zero / release lenght (voice 1)
    lda #33                                       // sawtooth wave, and sound on
    sta $d404                                     // set sawtooth (bit 5) and gate bit 0 to 1 (sound on)
    lda #70                                       // Delay 70... hamsters
    sta DELAY                                     // Store 70 in the DELAY variable
    jsr !delay+                                   // and call the delay subroutine
    lda #32                                       // 
    sta $d404                                     // set gate bit 0 to 0 (sound off)
    rts                                           // 
                                                  // 
//=========================================================================================================
//     MAIN MENU
//=========================================================================================================
!start_menu_screen:                               // 
    jsr !hide_cursor-                             // Cursor off
    jsr $E544                                     // Clear screen
    jsr !draw_top_menu_lines+                     // 
    rts                                           // 
                                                  // 
!goto_mainmenu:
!mainmenu:                                        //        
    lda #0                                        //
    sta INCONFIG                                  //
    lda #1                                        //
    sta MENU_ID                                   //
    jsr !start_menu_screen-                       // 
    lda #22 ; sta _LINE_POS_                      // Load 23 into accumulator and store it in zero page address $fb
    jsr !draw_menu_line+                          // Call the draw_menu_line sub routine to draw a line on row 23

    lda CONFIG_STATUS                             // Check the config status
    cmp #3                                        // WiFi configured or fully complete?
    bcs !all+                                     // Yes, show OSC menu items
    jmp !noconf+                                  // No, WiFi setup only
!all:                                             // 
    displayText(text_menu_item_6_osc_setup,7,3)   // [F3] - OSC Setup
    displayText(text_menu_item_8_tasmota,9,3)     // [F4] - Tasmota Setup
    displayText(text_menu_item_5_osc,11,3)        // [F5] - Cue list mode
    displayText(text_menu_item_7_buskmode,13,3)   // [F7] - Busk mode
!noconf:                                          // 
    displayText(text_main_menu,1,15)              // Text MAIN MENU
    displayText(text_menu_item_1,5,3)             // [F1] - WiFi Setup
    lda VICEMODE                                  // check vice mode                                    
    cmp #1                                        // do not display the ESP32 version in vice mode
    beq !keyinput+                                //
!keyinput:                                        // 
    jsr $ffe4                                     // Call KERNAL routine: Get character from keyboard buffer                  
!:  cmp #133                                      // F1 key pressed?
    bne !F5+                                      // No, next
    jmp !wifi_setup+                              // Yes, go to WiFi setup
!F5:cmp #135                                      // F5 key pressed?
    bne !F6+                                      // No, next

    jmp !main_osc_screen+                         // Yes, goto OSC send screen
!F6:cmp #134                                      // F3 Pressed?
    bne !F7+                                      // No, next.
   
                                     // WiFi configured or better                               // No, back to keyinput
    jmp !osc_setup+                        // Yes, goto OSC setup screen
!F7:cmp #136                                      // F7 key pressed?
    bne !F8+                                      // No, next

    jmp !buskmode+                               // Yes, exit menu
!F8:cmp #138                                      // F4 key pressed?
    bne !FX+                                      // No, next
    jmp !tasmota_setup+                           // Jump to Tasmota setup screen
!FX:jmp !keyinput-                                // Ignore all other keys and wait for user input
                                                  // 
//=========================================================================================================
//    MENU WIFI SETUP
//=========================================================================================================
// send byte 248 to get the wifi connection status
// send byte 251 to get current wifi ssid, password and time offset
// send byte 252 to set new ssid, password and time offset
//=========================================================================================================
!wifi_setup:                                      //     
    lda #1
    sta INCONFIG
    lda #255                                      //
    sta DELAY                                     //
    jsr !start_menu_screen-                       // 
    lda #10 ; sta _LINE_POS_                      // Load 8 into accumulator and store it in zero page address $fb
    jsr !draw_menu_line+                          // Call draw_menu_line sub routine to draw a line on row 8
    lda #20 ; sta _LINE_POS_                      // Load 20 into accumulator and store it in zero page address $fb
    jsr !draw_menu_line+                          // Call draw_menu_line sub routine to draw a line on row 20
                                                  // 
    displayText(text_wifi_menu,1,15)              // Display the menu title on line 1, row 15
    displayText(text_wifi_ssid,4,1)               // Display static text "SSID:" on line 4, row 1 
    displayText(text_wifi_password,6,1)           // Display static text "Password:" on line 6, row 1 
    displayText(text_time_offset,8,1)             //                
    //displayText(fakeline,22,1) 
    jsr !display_F7_menuItem+                     // Display "[ F7 ] exit menu" on line 17, row 3. 
    lda VICEMODE                                  // If the cartridge is not attached, then                                  
    cmp #1                                        // skip the part where we ask for wifi status          
    bne !+                                        // see if the vicemode variable is 1
    jmp !notVice+                                 // if so jump to not-vice                                                   
!:  jsr !get_wifi_status+                         // get the wifi status into the buffer  
!:  displayText(RXBUFFER,23,3)                    // Display the buffer on screen
    lda RXBUFFER                                  // 
    cmp #146                                      // Buffer starts with color code RED (because connection failed)
    beq !wifi_error+                              // 
    jmp !wifi_okay+                               // 
!wifi_error:                                      // 
    jsr !sounderror-                              // 
    jmp !continue+                                // 
!wifi_okay:                                       // 
    playsong(song2)                               // 
!continue:                                        // 
    lda #251                                      // Load number #251  (ask for WiFi SSID)
    sta CMD                                       // Store that in variable CMD
    jsr !send_start_byte_ff+                      // Call the sub routine to send 251 to the esp32
    lda TIMOUTERROR                               //
    cmp #1                                        //
    bne !+                                        //           
    jsr !delay255+                                //
    jmp !continue-                                //                                              
                                                  //
!:  lda #1                                        // the RXBUFFER now contains ssid[32]password[32]timeoffset[128]
    sta $02                                       //
    jsr !splitRXbuffer+                           //
    displayText(SPLITBUFFER,4,7)                  // Display the buffers on screen (SSID name)
                                                  //
    lda #2                                        //
    sta $02                                       //
    jsr !splitRXbuffer+                           //
    displayText(SPLITBUFFER,6,11)                 // Display the buffers on screen (password)
                                                  //
    lda #3                                        //
    sta $02                                       //
    jsr !splitRXbuffer+                           //
    displayText(SPLITBUFFER,8, 23)                // Display the buffers on screen (time offset from GMT)
                                                  //                                                  
!notVice:                                         // 
    lda #156                                      // Set the limits to where the cursor can travel
    sta CURSORCOLOR                               //
    lda #4                                        // Load 4 into accumulator
    sta HOME_LINE                                 // Store 4 into Home_line variable, so the cursor van not go above line 4
    sta LIMIT_LINE                                // Store 4 into limit_line variable so the cursor van not go below line 4
    lda #7                                        // Load 7 into accumulator
    sta HOME_COLM                                 // Store 7 into home_column variable, so the cursor can not go below 7
    lda #39                                       // Load 39 into accumulator
    sta LIMIT_COLM                                // Store 35 into the limit_column so the cursor can not go beyond that position
    lda #1                                        //
    sta MENU_ID                                   //
    sta CLEAR_FIELD_FLAG                          // SET clear text flag to 1 (default is zero)
    jsr !config_text_input+                              // Call the text input routine, we will be back when the user presses RETURN
                                                  // 
    lda #31                                       // Set the limits to where the cursor can travel
    sta CURSORCOLOR                               //
    lda #6                                        // Load 6 into accumulator
    sta LIMIT_LINE                                // Store 6 into limit_line variable so the cursor van not go below line 24
    sta HOME_LINE                                 // Store 6 into Home_line variable, so the cursor van not go above line 22
    lda #11                                       // Load 11 into accumulator
    sta HOME_COLM                                 // Store 11 into home_column variable, so the cursor can not go below 10
    lda #1                                        // Load 1 into the accumulator
    sta CLEAR_FIELD_FLAG                          // SET clear text flag to 1 (default is zero)
    jsr !config_text_input+                              // Call text input routine, we will be back when the user presses RETURN
                                                  //
    lda #149                                      // Set the limits to where the cursor can travel
    sta CURSORCOLOR                               //
    lda #8                                        // Load 8 into accumulator
    sta LIMIT_LINE                                // Store 8 into limit_line variable so the cursor van not go below line 24
    sta HOME_LINE                                 // Store 8 into Home_line variable, so the cursor van not go above line 22
    lda #23                                       // Load 14 into accumulator
    sta HOME_COLM                                 // Store 14 into home_column variable, so the cursor can not go below 10
    lda #1                                        // Load 1 into the accumulator
    sta CLEAR_FIELD_FLAG                          // SET clear text flag to 1 (default is zero)
    jsr !config_text_input+                              // Call text input routine, we will be back when the user presses RETURN 
    jsr !hide_cursor-                             // Hide the cursor
                                                  // 
    displayText(text_save_settings,14,3)          // 
                                                  // 
!keyinput:                                        // At this point the user can select F1 or F7 to Save settings and Test settings, or exit the menu
                                                  // 
    jsr !wait_for_a_key+                          //
    cmp #133                                      // F1 key pressed?
    beq !save_settings+                           // If true, save the WiFi settings
    cmp #136                                      // F7 key pressed?
    beq !exit_menu+                               // If true, exit to main menu
    jmp !keyinput-                                // Ignore all other keys and wait for user input again
                                                  // 
!exit_menu:                                       // F7 Pressed!
    jmp !config_f7_exit+                           // Clean exit to main menu
                                                  // 
!save_settings:                                   // F1 pressed, we are going to save the WiFi settings
    lda VICEMODE                                  // \                                              
    cmp #1                                        //  \ Skip save settings if we run in simulation without Cartrdige         
    bne !readSsid+                                //  /   
    jmp !wifi_setup-                              // /
!readSsid:                                        //
                                                  // \
    lda #$A7                                      //  \
    sta _FIELD_                                   //   The ssid is at screen memory location: $04A7
    lda #$04                                      //  /
    sta _FIELD_ +1                                // /
    lda #33                                       // set the number of character to read from screen
    sta READLIMIT                                 //
    jsr !read_from_screen+                        // Read the SSID Name from screen into the TXBUFFER
    jsr !wait_for_ready_to_receive+               // At this point we have the SSID name, from the screen, in the txbuffer
    lda #252                                      // Load 252 into accumulator
    sta _IO1_                                     // Send the start byte (252 = send wifi information)
    jsr !send_buffer+                             // Send the new SSID to the ESP32
!readPassword:                                    //
    lda #$FB                                      // - The password is at screen memory location: $4FB
    sta _FIELD_                                   // 
    lda #28                                       // set the number of character to read from screen
    sta READLIMIT                                 //
    jsr !read_from_screen+                        // Read the PASSWORD from screen into the TXBUFFER
                                                  // At this point we have the password, from the screen, in the txbuffer
    jsr !wait_for_ready_to_receive+               // Prepare the ESP to receive
    jsr !send_buffer+                             // Send the new password to the ESP32
                                                  //
!readTimeOffset:                                  //
    inc _FIELD_ +1                                // Next read the time offset from the screen
    lda #$57                                      // the time offset starts at $0557
    sta _FIELD_                                   // 
    lda #5                                        // set the number of character to read from screen
    sta READLIMIT                                 //
    jsr !read_from_screen+                        // read it
    jsr !wait_for_ready_to_receive+               // Prepare the ESP to receive
    jsr !send_buffer+                             // Send the new time offset to the ESP32
                                                  //
    ldx #23 ; jsr $e9ff                           // Clear display line 23 (where the connection status is)
                                                  //                                                      
                                                  // Now we need to wait a few seconds so the ESP can restart wifi with the new credentials
    lda #58                                       // setup the count down timer, ascii code for ':' (one above 9)
    sta text_wifi_wait_countdown                  // we count down from 9 to 0
!waitforwifi:                                     // this is a 10x loop to wait for wifi
    jsr !delay255+                                // call the delay subroutine    
    jsr !get_wifi_status+                         //
    lda RXBUFFER                                  // 
    cmp #149                                      // Buffer starts with color code GREEN if connection was succesful
    beq !endwait+                                 // once we have wifi, break out of the loop     
    dec text_wifi_wait_countdown                  // decrease the count down, 
    displayText(text_wifi_wait,23,3)              // Display the text "wait for wifi connection
    lda text_wifi_wait_countdown                  // check the counter (ascii code 47 is one below 0)
    cmp #47                                       // if we fall below zero, exit the loop
    bne !waitforwifi-                             //
!endwait:
    ldx #23 ; jsr $e9ff                           // Clear line 23 (where the connection status is)
    jsr !callstatus+                              // Check the configuration status
    lda CONFIG_STATUS                             // 
    cmp #3                                        // Config already done?
    bcs !+                                        // if so, we do not have to alter the status
    lda #4                                        // Mark WiFi + OSC ready for main menu
    jsr !sendstatus+                              // 
!:  jmp !wifi_setup-                              // Jump to start WiFi setup

!get_wifi_status:
!:  lda #248                                      // Load number #248 (ask for WiFi status)
    sta CMD                                       // Store that in CMD variable
    jsr !send_start_byte_ff+                      // Call the sub routine to send 248 to the esp32
    lda TIMOUTERROR                               // 
    cmp #1                                        //
    bne !+                                        //           
    jsr !delay255+                                //
    jmp !-                                        //                                                  
!:  rts 
//=========================================================================================================
//    SUB ROUTINE TO SPLIT RXBUFFER
//=========================================================================================================
!splitRXbuffer:                                   //
                                                  // RXBUFFER now contains FOR EXAMPLE macaddress[129]regid[129]nickname[129]regstatus[128]
    ldx #0                                        // load zero into x and y    
    ldy #0                                        //   
    lda RXBUFFER,x                                // See if the RXBUFFER is empty
    cmp #128                                      // exit if it is..
    beq !exit+                                    //
!read:                                            // read a byte from the index buffer   
    lda RXBUFFER,x                                // copy that byte to the split buffer   
    sta SPLITBUFFER,y                             // until we find byte 129   
    cmp #129                                      //    
    beq !+                                        //    
    cmp #128                                      // or the end of line character   
    beq !+                                        //    
    inx                                           // increase the x index   
    iny                                           // and also the y index   
    jmp !read-                                    // back to the start to get the next character   
!:                                                //    
    lda #128                                      //     
    sta SPLITBUFFER,y                             // load 128 (the end byte) into the splitbuffer   
    dec $02                                       // decrease $02. This address holds a number that indicates   
    lda $02                                       // which word we need from the RXBUFFER   
    cmp #0                                        // so if $02 is equal to zero, we have the right word   
    beq !exit+                                    // exit in that case   
    ldy #0                                        // if we need the next word   
    inx                                           // we reset the y index,   
    jmp !read-                                    // increase the x index   
                                                  // and get the next word from the RX buffer
!exit:                                            // 
    rts                                           // return.   
//=========================================================================================================
// Check if we have Wifi
//=========================================================================================================
!checkWiFi:                                       // 
    lda VICEMODE                                  //
    cmp #1                                        //
    beq !weHave_wifi+                             //
    lda #0                                        //
    sta HAVEWIFI                                  //
    lda #248                                      //
    sta CMD                                       // Store that in variable CMD
    jsr !send_start_byte_ff+                      // Call the sub routine to send 248 to the esp32 to ask for the wifi
    lda RXBUFFER                                  // the first byte of RXBUFFER now contains 146 or 149
    cmp #149                                      // for Not connected or Connected
    bne !+                                        //
!weHave_wifi:                                     //
    lda #1                                        //                                           
    sta HAVEWIFI                                  // We are connected, just return
!:  rts                                           //

//=========================================================================================================
//     OSC SEND SCREEN
//=========================================================================================================
!main_osc_screen:                                 // 
    jsr $e544
!osc_screen:                                      // Draw the divider line
    lda #6                                        //
    sta MENU_ID                                   //
    lda #02                                       // color number 12 is gray
    sta _LINE_COLOR_                              // store the color code in $c9
    lda #$0c
    sta $286
    
        lda #$00
        sta $d021
        sta $d021

        jsr !hide_cursor-


        ldx #$00
!:      lda text_osc_cuelist,x
        sta $0400,x
        lda text_osc_cuelist+$100,x
        sta $0500,x
        lda text_osc_cuelist+$200,x
        sta $0600,x                

        lda #$0c
        sta $d800,x
        sta $d900,x
        sta $da00,x
        sta $db00,x
        inx
        bne !-

!sendscreen:
    // Initialize cuecount to 0 if this is first entry (optional safety check)
    // cuecount should already be set, but ensure it's valid
    lda cuecount
    cmp #26                                              // If >= 26, wrap to 0
    bcc !+
    lda #0
    sta cuecount
!:
    ldy cuecount
    lda cue_lsb_table,y
    sta load_cue+1
    clc
    adc #81
    sta cue_card_addr
    lda cue_hsb_table,y
    sta load_cue+2
    adc #$00
    sta cue_card_addr+1

    ldx #$00
load_cue:
    lda list_item_1,x                                    // Address modified by code above (load_cue+1 and load_cue+2)
    sta TXBUFFER,x
    cmp #128
    beq !cue_done+                                       // Found end marker, exit loop
    ora #%10000000                                       // Set bit 7 for screen display (inverted)
    sta $0630,x
    lda #$07                                             // Yellow color for display
    sta $da30,x
    inx
    cpx #81                                              // Safety limit: max 81 bytes
    bne load_cue
    // If we reach here, no 128 byte was found - add it as end marker
    lda #128
    sta TXBUFFER,x
    inx

!cue_done:                                               // Exit point after loading cue (end marker found or added)
    lda cue_card_addr                                    // Load the base address (address + 81)
    sta $fc                                              // Store in zero page pointer
    lda cue_card_addr+1
    sta $fd
    ldy #0                                               // Load byte 0 of the indicator (at offset 81 from cue start)
    lda ($fc),y
    sta $05f9
    
    // Load next cue data for display
    ldy cuecount                                         // Get current cue index
    iny                                                  // Next cue index
    cpy #26                                              // Check if we've wrapped past the last cue
    bne !+
    ldy #0                                               // Wrap to first cue
!:  lda cue_lsb_table,y                                  // Get base address of next cue
    sta load_next_cue+1                                  // Modify instruction to load from next cue
    lda cue_hsb_table,y
    sta load_next_cue+2

    ldx #$00
load_next_cue:
    lda list_item_2,x                                    // Address modified by code above (load_next_cue+1 and load_next_cue+2)
    cmp #128                                             // Check for end marker
    beq !next_cue_done+                                  // Exit if end marker found
    ora #%10000000                                       // Set bit 7 for screen display (inverted)
    sta $06f8,x                                          // Display next cue on screen
    lda #$07                                             // Yellow color for display
    sta $db78,x                                          // Color memory for next cue (offset from $06f8)
    inx
    cpx #80                                              // Safety limit
    bne load_next_cue
    // If we reach here, no 128 byte was found - skip rest
!next_cue_done:

nxt_cue_card:
    // Get indicator byte for next cue (at offset 81 from next cue's base address)
    ldy cuecount                                         // Get current cue index
    iny                                                  // Next cue index  
    cpy #26                                              // Check if we've wrapped past the last cue
    bne !+
    ldy #0                                               // Wrap to first cue
!:  lda cue_lsb_table,y                                  // Get base address of next cue
    clc
    adc #81                                              // Add offset to indicator byte (byte 81 in the cue data)
    sta $fc
    lda cue_hsb_table,y
    adc #$00
    sta $fd
    ldy #0                                               // Load byte 0 of the indicator area
    lda ($fc),y
    sta $06bf                                            // Display next cue indicator    

!keyloop:
    jsr $ffe4
    beq !keyloop-                                     // Loop if no key pressed
    
    cmp #$0d                                          // Enter key - send current cue
    beq !send_cue+
    cmp #$91                                          // Cursor up - move to previous cue ($91 = cursor up)
    beq !cursor_up+
    cmp #$11                                          // Cursor down - move to next cue ($11 = cursor down)
    bne !+
    jmp !cursor_down+
!:
    cmp #$86                                          // F3 - toggle edit mode
    bne !+
    jmp editmode
!:  cmp #$85                                          // F1 - toggle busk mode
    bne !+
    jmp buskmode
!:  cmp #$88                                          // F7 - exit
    bne !+
    jmp exit_osc
!:
    jmp !keyloop-

exit_osc:
    lda #$00
    sta $d020
    jmp !mainmenu-

!send_cue:
    // Copy displayed cue (with inverted bit 7) to Active Cue area on screen
    ldx #$00
!copy_loop:
    lda $0630,x                                    // Load from displayed armed cue area
    sta $0568,x                                    // Copy to Active Cue display area
    lda #$05                                       // Yellow color
    sta $d968,x
    inx
    cpx #80
    bne !copy_loop-
    
    lda $05f9                                      // Copy indicator
    sta $0531
    
    // TXBUFFER already contains the cue data (loaded earlier without bit 7)
    // Verify it has the 128 end marker
    ldx #$00
!find_end:
    lda TXBUFFER,x
    cmp #128
    beq !send+
    inx
    cpx #81                                        // Safety limit
    bne !find_end-
    // If we get here, no end marker found - add it
    lda #128
    sta TXBUFFER,x
        
!send:
    jsr !wait_for_ready_to_receive+               // Prepare ESP32 to receive
    lda #226                                      // Load 226 into accumulator (send OSC message)
    sta _IO1_                                     // Send the start byte (226 = send new osc message)
    jsr !send_buffer+                             // Send the message to the ESP32
    inc cuecount
    lda cuecount
    cmp #26
    bne !+
    lda #$00
    sta cuecount
 !: jmp !sendscreen-

!cursor_up:                                           // Move to previous cue
    lda cuecount
    beq !+                                            // If at 0, wrap to max
    dec cuecount
    jmp !sendscreen-
!:  lda #25                                           // Wrap to cue 25 (0-25 = 26 cues)
    sta cuecount
    jmp !sendscreen-

!cursor_down:                                         // Move to next cue
    inc cuecount
    lda cuecount
    cmp #26                                           // If past 25, wrap to 0
    bne !+
    lda #0
    sta cuecount
!:  jmp !sendscreen-

buskmode: jmp !buskmode+
// --------------------------------------------------------------------------------------------------
editmode:

        jsr $e544
        lda #$00
        sta $d020
        ldx #$00
!:      lda text_osc_editlist,x
        sta $0400,x
        lda text_osc_editlist+$100,x
        sta $0500,x
        lda text_osc_editlist+$200,x
        sta $0600,x                

        lda #$0c
        sta $d800,x
        sta $d900,x
        sta $da00,x
        sta $db00,x
        inx
        bne !-
        lda cuecount
        sta editcount
loop_write:
    ldy editcount
    lda cue_lsb_table,y
    sta load_edit+1
    clc
    adc #81
    sta cue_edit_card+1
    lda cue_hsb_table,y
    sta load_edit+2
    adc #$00
    sta cue_edit_card+2


cue_edit_card:
    lda list_item_1
    sta $05f9

    ldx #$00
load_edit:
    lda list_item_1,x
    cmp #128
    beq !+
    sta $0630,x
    lda #$07
    sta $da30,x
    inx
    cpx #80
    bne load_edit        
    

    lda #14                                        // Load 14 into accumulator
    sta HOME_LINE                                 // Store 14 into Home_line variable, so the cursor can not go above line 14
    lda #0                                        // Load 0 into accumulator
    sta HOME_COLM                                 // Store 0 into home_column variable, so the cursor can not go below column 0
    lda #15                                        // Load 15 into accumulator
    sta LIMIT_LINE                                // Store 15 into limit_line variable so the cursor can not go below line 15
    lda #39                                       // Load 39 into accumulator
    sta LIMIT_COLM                                // Store 39 into the limit_column so the cursor can not go beyond that position
    lda #0                                        // Load 0 into accumulator
    sta CLEAR_FIELD_FLAG                          // and SET clear text flag to 0 (default is zero)
    lda #158                                       // Load 158 (yellow) for cursor color
    sta CURSORCOLOR                               //
    lda #6                                        // Set MENU_ID to 6 so exit_F3 knows we're in OSC edit mode
    sta MENU_ID                                   //
    ldx HOME_LINE
    
    
    jsr !config_text_input+                              // Call the text input routine, we will be back when the user presses RETURN or F3 
    
    ldx #$00
    ldy editcount
    lda cue_lsb_table,y
    sta store_new_cue+1
    lda cue_hsb_table,y
    sta store_new_cue+2
!:
    lda $0630,x
    and #%01111111
store_new_cue:

    sta list_item_1,x
    inx
    cpx #80
    bne !-
    inc editcount
    lda editcount
    cmp #26
    bne !+
    lda #$00
    sta editcount
!:
    jmp loop_write    


    
//----------------------------------------------------------------------------------

!buskmode:
	// set to 25 line text mode and turn on the screen
	lda #$1B
	sta $D011

	// disable SHIFT-Commodore
	lda #$80
	sta $0291

	// set screen memory ($0400) and charset bitmap offset ($2000)
	lda #$17
	sta $D018

	// set border color
	lda #$00
	sta $D020
	
	// set background color
	lda #$00
	sta $D021
    jsr !hide_cursor-
	jsr $e544

    ldx #$00
!:    
    lda #$0c
    sta $d800,x
    sta $d900,x
    sta $da00,x
    sta $db00,x
    lda text_osc_buskscreen,x
    sta $0400,x
    lda text_osc_buskscreen+$68,x
    sta $0400+$68,x

    inx
    bne !-

!loop:
	jsr $ffe4

	cmp #$41
	bne !+
	jmp fire1
!:	cmp #$42
	bne !+
	jmp fire2
!: cmp #$43
	bne !+
	jmp fire3
!:	cmp #$44
	bne !+
	jmp fire4
!:	cmp #$45
	bne !+
	jmp fire5
!:	cmp #$46
	bne !+
	jmp fire6
!:	cmp #$47
	bne !+
	jmp fire7
!: cmp #$48
	bne !+
	jmp fire8
!:	cmp #$49
	bne !+
	jmp fire9
!:	cmp #$4a
	bne !+
	jmp fire10
!:	cmp #$4b
	bne !+
	jmp fire11
!:	cmp #$4c
	bne !+
	jmp fire12
!: cmp #$4d
	bne !+
	jmp fire13
!:	cmp #$4e
	bne !+
	jmp fire14
!:	cmp #$4f
	bne !+
	jmp fire15
!:	cmp #$50
	bne !+
	jmp fire16
!:	cmp #$51
	bne !+
	jmp fire17
!: cmp #$52
	bne !+
	jmp fire18
!:	cmp #$53
	bne !+
	jmp fire19
!:	cmp #$54
	bne !+
	jmp fire20
!:	cmp #$55
	bne !+
	jmp fire21
!:	cmp #$56
	bne !+
	jmp fire22
!: cmp #$57
	bne !+
	jmp fire23
!:	cmp #$58
	bne !+
	jmp fire24
!:	cmp #$59
	bne !+
	jmp fire25
!:	cmp #$5a
	bne !+
	jmp fire26	
!:	cmp #$C1                                      // Shift+A through Shift+Z ($C1-$DA)
	bcc !+
	cmp #$DB
	bcs !+
	sec
	sbc #$C1                                      // Index 0-25
	tax
	lda cue_shift_lo,x
	sta osc_print+1
	lda cue_shift_hi,x
	sta osc_print+2
	jmp fire_osc
!:	cmp #$20
	bne !+
	jmp fire_space
!:	cmp #$30
	bne !+
	jmp fire30
!:	cmp #$31
	bne !+
	jmp fire31
!:	cmp #$32
	bne !+
	jmp fire32
!:	cmp #$33
	bne !+
	jmp fire33
!:	cmp #$34
	bne !+
	jmp fire34
!:	cmp #$35
	bne !+
	jmp fire35
!:	cmp #$36
	bne !+
	jmp fire36
!:	cmp #$37
	bne !+
	jmp fire37
!:	cmp #$38
	bne !+
	jmp fire38
!:	cmp #$39
	bne !+
	jmp fire39                                        
!:  cmp #$85                                      // F1 - toggle busk mode
    bne !+
    jmp !main_osc_screen-                         // Jump back to OSC send screen (cuelist mode)
!:  cmp #$88                                      // F1 - toggle busk mode
    bne !+
    jmp !mainmenu-   
!: 
    jmp !loop-	

fire_osc:
	ldx #$00
!print:	
osc_print:
	lda cue_1,x                                          // Address modified by fire routines (osc_print+1 and osc_print+2)
    sta TXBUFFER,x
    cmp #128
    beq !done_print+                                     // Found end marker, exit loop
	sta outputline,x
    lda #$05
    sta outputline+$d400,x
	inx
	cpx #81                                              // Safety limit
	bne !print-
    // If we get here, no 128 found - add it
    lda #128
    sta TXBUFFER,x
    
!done_print:
    ldx #$00
inv:
    lda outputline,x
    ora #%10000000
    sta outputline,x
    inx
    cpx #80
    bne inv

!:  jsr sendbusk
	jmp !loop-

fire1:

    lda #<cue_1
    sta osc_print+1
    lda #>cue_1
    sta osc_print+2
    jmp fire_osc

fire2:

    lda #<cue_2
    sta osc_print+1
    lda #>cue_2
    sta osc_print+2
    jmp fire_osc

fire3:

    lda #<cue_3
    sta osc_print+1
    lda #>cue_3
    sta osc_print+2
    jmp fire_osc

fire4:

    lda #<cue_4
    sta osc_print+1
    lda #>cue_4
    sta osc_print+2
    jmp fire_osc

fire5:

    lda #<cue_5
    sta osc_print+1
    lda #>cue_5
    sta osc_print+2
    jmp fire_osc

fire6:

    lda #<cue_6
    sta osc_print+1
    lda #>cue_6
    sta osc_print+2
    jmp fire_osc

fire7:

    lda #<cue_7
    sta osc_print+1
    lda #>cue_7
    sta osc_print+2
    jmp fire_osc

fire8:

    lda #<cue_8
    sta osc_print+1
    lda #>cue_8
    sta osc_print+2
    jmp fire_osc

fire9:

    lda #<cue_9
    sta osc_print+1
    lda #>cue_9
    sta osc_print+2
    jmp fire_osc

fire10:

    lda #<cue_10
    sta osc_print+1
    lda #>cue_10
    sta osc_print+2
    jmp fire_osc

fire11:

    lda #<cue_11
    sta osc_print+1
    lda #>cue_11
    sta osc_print+2
    jmp fire_osc

fire12:

    lda #<cue_12
    sta osc_print+1
    lda #>cue_12
    sta osc_print+2
    jmp fire_osc

fire13:

    lda #<cue_13
    sta osc_print+1
    lda #>cue_13
    sta osc_print+2
    jmp fire_osc

fire14:

    lda #<cue_14
    sta osc_print+1
    lda #>cue_14
    sta osc_print+2
    jmp fire_osc

fire15:

    lda #<cue_15
    sta osc_print+1
    lda #>cue_15
    sta osc_print+2
    jmp fire_osc

fire16:

    lda #<cue_16
    sta osc_print+1
    lda #>cue_16
    sta osc_print+2
    jmp fire_osc

fire17:

    lda #<cue_17
    sta osc_print+1
    lda #>cue_17
    sta osc_print+2
    jmp fire_osc

fire18:

    lda #<cue_18
    sta osc_print+1
    lda #>cue_18
    sta osc_print+2
    jmp fire_osc

fire19:

    lda #<cue_19
    sta osc_print+1
    lda #>cue_19
    sta osc_print+2
    jmp fire_osc

fire20:

    lda #<cue_20
    sta osc_print+1
    lda #>cue_20
    sta osc_print+2
    jmp fire_osc

fire21:

    lda #<cue_21
    sta osc_print+1
    lda #>cue_21
    sta osc_print+2
    jmp fire_osc

fire22:

    lda #<cue_22
    sta osc_print+1
    lda #>cue_22
    sta osc_print+2
    jmp fire_osc

fire23:

    lda #<cue_23
    sta osc_print+1
    lda #>cue_23
    sta osc_print+2
    jmp fire_osc

fire24:

    lda #<cue_24
    sta osc_print+1
    lda #>cue_24
    sta osc_print+2
    jmp fire_osc

fire25:

    lda #<cue_25
    sta osc_print+1
    lda #>cue_25
    sta osc_print+2
    jmp fire_osc

fire26:

    lda #<cue_26
    sta osc_print+1
    lda #>cue_26
    sta osc_print+2
    jmp fire_osc

fire30:

    lda #<cue_30
    sta osc_print+1
    lda #>cue_30
    sta osc_print+2
    jmp fire_osc

fire31:

    lda #<cue_31
    sta osc_print+1
    lda #>cue_31
    sta osc_print+2
    jmp fire_osc

fire32:

    lda #<cue_32
    sta osc_print+1
    lda #>cue_32
    sta osc_print+2
    jmp fire_osc

fire33:

    lda #<cue_33
    sta osc_print+1
    lda #>cue_33
    sta osc_print+2
    jmp fire_osc

fire34:

    lda #<cue_34
    sta osc_print+1
    lda #>cue_34
    sta osc_print+2
    jmp fire_osc

fire35:

    lda #<cue_35
    sta osc_print+1
    lda #>cue_35
    sta osc_print+2
    jmp fire_osc

fire36:

    lda #<cue_36
    sta osc_print+1
    lda #>cue_36
    sta osc_print+2
    jmp fire_osc

fire37:

    lda #<cue_37
    sta osc_print+1
    lda #>cue_37
    sta osc_print+2
    jmp fire_osc

fire38:

    lda #<cue_38
    sta osc_print+1
    lda #>cue_38
    sta osc_print+2
    jmp fire_osc

fire39:

    lda #<cue_39
    sta osc_print+1
    lda #>cue_39
    sta osc_print+2
    jmp fire_osc

fire_space:

    lda #<cue_space
    sta osc_print+1
    lda #>cue_space 
    sta osc_print+2
    jmp fire_osc

sendbusk:
    jsr !wait_for_ready_to_receive+               // At this point we have the chat message from the screen, in the txbuffer
    lda #226                                      // Load 226 into accumulator    
    sta _IO1_                                     // Send the start byte (226 = send new osc message)
    jsr !send_buffer+                             // Send the message to the ESP32
    rts

cue_1:
.text "/c64/key/a               "
.byte 128
cue_2:
.text "/c64/key/b               "
.byte 128
cue_3:
.text "/c64/key/c               "
.byte 128
cue_4:
.text "/c64/key/d               "
.byte 128
cue_5:
.text "/c64/key/e               "
.byte 128
cue_6:
.text "/c64/key/f               "
.byte 128
cue_7:
.text "/c64/key/g               "
.byte 128
cue_8:
.text "/c64/key/h               "
.byte 128
cue_9:
.text "/c64/key/i               "
.byte 128
cue_10:
.text "/c64/key/j               "
.byte 128
cue_11:
.text "/c64/key/k               "
.byte 128
cue_12:
.text "/c64/key/l               "                              
.byte 128
cue_13:
.text "/c64/key/m               "
.byte 128
cue_14:
.text "/c64/key/n               "
.byte 128
cue_15:
.text "/c64/key/o               "
.byte 128
cue_16:
.text "/c64/key/p               "
.byte 128
cue_17:
.text "/c64/key/q               "
.byte 128
cue_18:
.text "/c64/key/r               "
.byte 128
cue_19:
.text "/c64/key/s               "
.byte 128
cue_20:
.text "/c64/key/t               "
.byte 128
cue_21:
.text "/c64/key/u               "
.byte 128
cue_22:
.text "/c64/key/v               "
.byte 128
cue_23:
.text "/c64/key/w               "
.byte 128
cue_24:
.text "/c64/key/x               "
.byte 128
cue_25:
.text "/c64/key/y               "
.byte 128
cue_26:
.text "/c64/key/z               "
.byte 128

cue_shift_1:
.text "/c64/key/shift/a         "
.byte 128
cue_shift_2:
.text "/c64/key/shift/b         "
.byte 128
cue_shift_3:
.text "/c64/key/shift/c         "
.byte 128
cue_shift_4:
.text "/c64/key/shift/d         "
.byte 128
cue_shift_5:
.text "/c64/key/shift/e         "
.byte 128
cue_shift_6:
.text "/c64/key/shift/f         "
.byte 128
cue_shift_7:
.text "/c64/key/shift/g         "
.byte 128
cue_shift_8:
.text "/c64/key/shift/h         "
.byte 128
cue_shift_9:
.text "/c64/key/shift/i         "
.byte 128
cue_shift_10:
.text "/c64/key/shift/j         "
.byte 128
cue_shift_11:
.text "/c64/key/shift/k         "
.byte 128
cue_shift_12:
.text "/c64/key/shift/l         "
.byte 128
cue_shift_13:
.text "/c64/key/shift/m         "
.byte 128
cue_shift_14:
.text "/c64/key/shift/n         "
.byte 128
cue_shift_15:
.text "/c64/key/shift/o         "
.byte 128
cue_shift_16:
.text "/c64/key/shift/p         "
.byte 128
cue_shift_17:
.text "/c64/key/shift/q         "
.byte 128
cue_shift_18:
.text "/c64/key/shift/r         "
.byte 128
cue_shift_19:
.text "/c64/key/shift/s         "
.byte 128
cue_shift_20:
.text "/c64/key/shift/t         "
.byte 128
cue_shift_21:
.text "/c64/key/shift/u         "
.byte 128
cue_shift_22:
.text "/c64/key/shift/v         "
.byte 128
cue_shift_23:
.text "/c64/key/shift/w         "
.byte 128
cue_shift_24:
.text "/c64/key/shift/x         "
.byte 128
cue_shift_25:
.text "/c64/key/shift/y         "
.byte 128
cue_shift_26:
.text "/c64/key/shift/z         "
.byte 128

cue_shift_lo:
.byte <cue_shift_1, <cue_shift_2, <cue_shift_3, <cue_shift_4, <cue_shift_5, <cue_shift_6, <cue_shift_7, <cue_shift_8
.byte <cue_shift_9, <cue_shift_10, <cue_shift_11, <cue_shift_12, <cue_shift_13, <cue_shift_14, <cue_shift_15, <cue_shift_16
.byte <cue_shift_17, <cue_shift_18, <cue_shift_19, <cue_shift_20, <cue_shift_21, <cue_shift_22, <cue_shift_23, <cue_shift_24
.byte <cue_shift_25, <cue_shift_26
cue_shift_hi:
.byte >cue_shift_1, >cue_shift_2, >cue_shift_3, >cue_shift_4, >cue_shift_5, >cue_shift_6, >cue_shift_7, >cue_shift_8
.byte >cue_shift_9, >cue_shift_10, >cue_shift_11, >cue_shift_12, >cue_shift_13, >cue_shift_14, >cue_shift_15, >cue_shift_16
.byte >cue_shift_17, >cue_shift_18, >cue_shift_19, >cue_shift_20, >cue_shift_21, >cue_shift_22, >cue_shift_23, >cue_shift_24
.byte >cue_shift_25, >cue_shift_26

cue_31:
.text "/c64/key/1              "
.byte 128
cue_32:
.text "/c64/key/2              "
.byte 128
cue_33:
.text "/c64/key/3              "
.byte 128
cue_34:
.text "/c64/key/4              "
.byte 128
cue_35:
.text "/c64/key/5              "
.byte 128
cue_36:
.text "/c64/key/6              "
.byte 128
cue_37:
.text "/c64/key/7              "
.byte 128
cue_38:
.text "/c64/key/8              "
.byte 128
cue_39:
.text "/c64/key/9              "
.byte 128
cue_30:
.text "/c64/key/0              "
.byte 128
cue_space:
.text "/c64/key/space          "
.byte 128
//=========================================================================================================
//    MENU OSC SETUP SCREEN (F6)
//=========================================================================================================
// send byte 227 to get OSC server IP and port
// send byte 228 to set OSC server IP and port
// send byte 220 to load cue list
// send byte 221 to save cue list
//=========================================================================================================
//=========================================================================================================
//    MENU OSC SETUP
//=========================================================================================================
// send byte 229 to get the osc destination ip and port
//=========================================================================================================
!osc_setup:                                   // 
    lda #1
    sta INCONFIG
    lda #255                                      //
    sta DELAY                                     //
    lda #1
    sta MENU_ID
    jsr !checkWiFi-                               // check if we have wifi   
    jsr !start_menu_screen-                       // 
    displayText(text_osc_send_menu,1,15)           // Display the menu title on line 1, row 15,
!ac_wifi_check:                                   //
    lda HAVEWIFI                                  //
    cmp #1                                        //
    beq !+                                        //
    displayText(text_any_key,21,7)                //
    displayText(text_error_no_internet,4,1)       //
    jsr !wait_for_a_key+                          //
    jmp !mainmenu-                                //
!:
    displayText(text_osc1_ip,6,1)             // Display static text "osc ip:" on line 6, row 1,  
    displayText(text_osc1_port,7,1)           // Display static text "osc port:" on line 8, row 1,  
    displayText(text_osc2_ip,8,1)             // Display static text "osc ip:" on line 6, row 1,  
    displayText(text_osc2_port,9,1)           // Display static text "osc port:" on line 8, row 1,  
    displayText(text_osc3_ip,10,1)             // Display static text "osc ip:" on line 6, row 1,  
    displayText(text_osc3_port,11,1)           // Display static text "osc port:" on line 8, row 1,  
    displayText(text_osc4_ip,12,1)             // Display static text "osc ip:" on line 6, row 1,  
    displayText(text_osc4_port,13,1)           // Display static text "osc port:" on line 8, row 1,  
    displayText(text_osc5_ip,14,1)             // Display static text "osc ip:" on line 6, row 1,  
    displayText(text_osc5_port,15,1)           // Display static text "osc port:" on line 8, row 1,  

    lda #19 ; sta _LINE_POS_                      // Load 10 into accumulator and store it in zero page address $fb
    jsr !draw_menu_line+                          // Call the draw_menu_line sub routine to draw a line on row 10
    jsr !display_F7_menuItem+                     // Display "[ F7 ] exit menu" on line 15, row 1.  
                                                  //
    lda VICEMODE                                  // \                                  
    cmp #1                                        //  \ Skip receiving the data from the cartridge if the cartridge is not attachted! 
    bne !+                                        //  /  
    jmp !fill_fields+                             // /   
                                                  // 
!sendcmd:                                         // 
!:  lda #227                                      // load the number #227
    sta CMD                                       // Store that in variable CMD
    jsr !send_start_byte_ff+                      // Call the sub routine to send 231 to the esp32 to ask for the osc ip and port
    lda TIMOUTERROR                               // RXBUFFER now contains Osc_IP[15]Osc_port[5]
    cmp #1                                        //
    bne !+                                        //           
    jsr !delay+                                   //
    jmp !sendcmd-                                 //
                                                  //
!:  lda #1                                        // we need the first element from the RXBUFFER (osc ip)
    sta $02                                       // store 1 (1=first element) in $02
jsr !splitRXbuffer-                              // copy the first element to Splitbuffer
    displayText(SPLITBUFFER,6,19)                 // Display the buffer (containing osc ip) on screen
                                                  // 
    lda #2                                        // now we need the second element from the RX buffer (osc port)
    sta $02                                       // so we put #2 in address $02
    jsr !splitRXbuffer-                          // and call the split routine to copy the element to the Splitbuffer
    displayText(SPLITBUFFER,7,19)                 // Display the buffer (containing osc port) on screen

!:  lda #3                                        // we need the first element from the RXBUFFER (osc ip)
    sta $02                                       // store 1 (1=first element) in $02
jsr !splitRXbuffer-                              // copy the first element to Splitbuffer
    displayText(SPLITBUFFER,8,19)                 // Display the buffer (containing osc ip) on screen
                                                  // 
    lda #4                                        // now we need the second element from the RX buffer (osc port)
    sta $02                                       // so we put #2 in address $02
    jsr !splitRXbuffer-                          // and call the split routine to copy the element to the Splitbuffer
    displayText(SPLITBUFFER,9,19)                 // Display the buffer (containing osc port) on screen

!:  lda #5                                        // we need the first element from the RXBUFFER (osc ip)
    sta $02                                       // store 1 (1=first element) in $02
jsr !splitRXbuffer-                              // copy the first element to Splitbuffer
    displayText(SPLITBUFFER,10,19)                 // Display the buffer (containing osc ip) on screen
                                                  // 
    lda #6                                        // now we need the second element from the RX buffer (osc port)
    sta $02                                       // so we put #2 in address $02
    jsr !splitRXbuffer-                          // and call the split routine to copy the element to the Splitbuffer
    displayText(SPLITBUFFER,11,19)                 // Display the buffer (containing osc port) on screen

!:  lda #7                                        // we need the first element from the RXBUFFER (osc ip)
    sta $02                                       // store 1 (1=first element) in $02
jsr !splitRXbuffer-                              // copy the first element to Splitbuffer
    displayText(SPLITBUFFER,12,19)                 // Display the buffer (containing osc ip) on screen
                                                  // 
    lda #8                                        // now we need the second element from the RX buffer (osc port)
    sta $02                                       // so we put #2 in address $02
    jsr !splitRXbuffer-                          // and call the split routine to copy the element to the Splitbuffer
    displayText(SPLITBUFFER,13,19)                 // Display the buffer (containing osc port) on screen                              

!:  lda #9                                        // we need the first element from the RXBUFFER (osc ip)
    sta $02                                       // store 1 (1=first element) in $02
jsr !splitRXbuffer-                              // copy the first element to Splitbuffer
    displayText(SPLITBUFFER,14,19)                 // Display the buffer (containing osc ip) on screen
                                                  // 
    lda #10                                        // now we need the second element from the RX buffer (osc port)
    sta $02                                       // so we put #2 in address $02
    jsr !splitRXbuffer-                          // and call the split routine to copy the element to the Splitbuffer
    displayText(SPLITBUFFER,15,19)                 // Display the buffer (containing osc port) on screen                
                                                  // Now we need a text input box so the user can change the ip and port
!fill_fields:                                     // Set the limits to where the cursor can travel
    lda #5                                        // Load 2 into accumulator
    sta MENU_ID                                   // and store it as the ID of this menu
    lda #6                                        // Load 6 into accumulator
    sta HOME_LINE                                 // Store 6 into Home_line variable, so the cursor van not go above line 4
    sta LIMIT_LINE                                // Store 4 into limit_line variable so the cursor van not go below line 4
    lda #19                                       // Load 7 into accumulator
    sta HOME_COLM                                 // Store 7 into home_column variable, so the cursor can not go below 7
    lda #34                                       // Load 39 into accumulator
    sta LIMIT_COLM                                // Store 39 into the limit_column so the cursor can not go beyond that position
    lda #0                                        // Load 0 into accumulator - don't clear field so user can edit existing text
    sta CLEAR_FIELD_FLAG                          // SET clear text flag to 0 (keep existing text visible for editing)
    lda #31                                       //
    sta CURSORCOLOR                               //
    jsr !config_text_input+                              // Call the text input routine, we will be back when the user presses RETURN
    jsr invert_cursor
    lda #7                                        // Load 6 into accumulator
    sta HOME_LINE                                 // Store 6 into Home_line variable, so the cursor van not go above line 22
    sta LIMIT_LINE                                // Store 6 into limit_line variable so the cursor van not go below line 24
    lda #19                                       // Load 11 into accumulator
    sta HOME_COLM                                 // Store 11 into home_column variable, so the cursor can not go below 10
    lda #24                                       // Load 5 into accumulator
    sta LIMIT_COLM                                // Store 5 into the limit_column so the cursor can not go beyond that position
    lda #0                                        // Load 0 - don't clear field so user can edit existing text
    sta CLEAR_FIELD_FLAG                          // 
    lda #149                                      //
    sta CURSORCOLOR                               //
    jsr !config_text_input+                              // Call the text input routine, we will be back when the user presses RETURN
    jsr invert_cursor
        lda #8                                        // Load 6 into accumulator
    sta HOME_LINE                                 // Store 6 into Home_line variable, so the cursor van not go above line 4
    sta LIMIT_LINE                                // Store 4 into limit_line variable so the cursor van not go below line 4
    lda #19                                       // Load 7 into accumulator
    sta HOME_COLM                                 // Store 7 into home_column variable, so the cursor can not go below 7
    lda #34                                       // Load 39 into accumulator
    sta LIMIT_COLM                                // Store 39 into the limit_column so the cursor can not go beyond that position
    lda #0                                        // Load 0 - don't clear field so user can edit existing text
    sta CLEAR_FIELD_FLAG                          // SET clear text flag to 0 (keep existing text visible for editing)
    lda #31                                       //
    sta CURSORCOLOR                               //
    jsr !config_text_input+                              // Call the text input routine, we will be back when the user presses RETURN
    jsr invert_cursor
    lda #9                                        // Load 6 into accumulator
    sta HOME_LINE                                 // Store 6 into Home_line variable, so the cursor van not go above line 22
    sta LIMIT_LINE                                // Store 6 into limit_line variable so the cursor van not go below line 24
    lda #19                                       // Load 11 into accumulator
    sta HOME_COLM                                 // Store 11 into home_column variable, so the cursor can not go below 10
    lda #24                                       // Load 5 into accumulator
    sta LIMIT_COLM                                // Store 5 into the limit_column so the cursor can not go beyond that position
    lda #0                                        // Load 0 - don't clear field so user can edit existing text
    sta CLEAR_FIELD_FLAG                          // 
    lda #149                                      //
    sta CURSORCOLOR                               //
    jsr !config_text_input+                              // Call the text input routine, we will be back when the user presses RETURN
    jsr invert_cursor
    lda #10                                        // Load 6 into accumulator
    sta HOME_LINE                                 // Store 6 into Home_line variable, so the cursor van not go above line 4
    sta LIMIT_LINE                                // Store 4 into limit_line variable so the cursor van not go below line 4
    lda #19                                       // Load 7 into accumulator
    sta HOME_COLM                                 // Store 7 into home_column variable, so the cursor can not go below 7
    lda #34                                       // Load 39 into accumulator
    sta LIMIT_COLM                                // Store 39 into the limit_column so the cursor can not go beyond that position
    lda #0                                        // Load 0 - don't clear field so user can edit existing text
    sta CLEAR_FIELD_FLAG                          // SET clear text flag to 0 (keep existing text visible for editing)
    lda #31                                       //
    sta CURSORCOLOR                               //
    jsr !config_text_input+                              // Call the text input routine, we will be back when the user presses RETURN
    jsr invert_cursor
    lda #11                                       // Load 6 into accumulator
    sta HOME_LINE                                 // Store 6 into Home_line variable, so the cursor van not go above line 22
    sta LIMIT_LINE                                // Store 6 into limit_line variable so the cursor van not go below line 24
    lda #19                                       // Load 11 into accumulator
    sta HOME_COLM                                 // Store 11 into home_column variable, so the cursor can not go below 10
    lda #24                                       // Load 5 into accumulator
    sta LIMIT_COLM                                // Store 5 into the limit_column so the cursor can not go beyond that position
    lda #0                                        // Load 0 - don't clear field so user can edit existing text
    sta CLEAR_FIELD_FLAG                          // 
    lda #149                                      //
    sta CURSORCOLOR                               //
    jsr !config_text_input+                              // Call the text input routine, we will be back when the user presses RETURN    
    jsr invert_cursor
    lda #12                                        // Load 6 into accumulator
    sta HOME_LINE                                 // Store 6 into Home_line variable, so the cursor van not go above line 4
    sta LIMIT_LINE                                // Store 4 into limit_line variable so the cursor van not go below line 4
    lda #19                                       // Load 7 into accumulator
    sta HOME_COLM                                 // Store 7 into home_column variable, so the cursor can not go below 7
    lda #34                                       // Load 39 into accumulator
    sta LIMIT_COLM                                // Store 39 into the limit_column so the cursor can not go beyond that position
    lda #0                                        // Load 0 - don't clear field so user can edit existing text
    sta CLEAR_FIELD_FLAG                          // SET clear text flag to 0 (keep existing text visible for editing)
    lda #31                                       //
    sta CURSORCOLOR                               //
    jsr !config_text_input+                              // Call the text input routine, we will be back when the user presses RETURN
    jsr invert_cursor
    lda #13                                        // Load 6 into accumulator
    sta HOME_LINE                                 // Store 6 into Home_line variable, so the cursor van not go above line 22
    sta LIMIT_LINE                                // Store 6 into limit_line variable so the cursor van not go below line 24
    lda #19                                       // Load 11 into accumulator
    sta HOME_COLM                                 // Store 11 into home_column variable, so the cursor can not go below 10
    lda #24                                       // Load 5 into accumulator
    sta LIMIT_COLM                                // Store 5 into the limit_column so the cursor can not go beyond that position
    lda #0                                        // Load 0 - don't clear field so user can edit existing text
    sta CLEAR_FIELD_FLAG                          // 
    lda #149                                      //
    sta CURSORCOLOR                               //
    jsr !config_text_input+                              // Call the text input routine, we will be back when the user presses RETURN
    jsr invert_cursor
        lda #14                                        // Load 6 into accumulator
    sta HOME_LINE                                 // Store 6 into Home_line variable, so the cursor van not go above line 4
    sta LIMIT_LINE                                // Store 4 into limit_line variable so the cursor van not go below line 4
    lda #19                                       // Load 7 into accumulator
    sta HOME_COLM                                 // Store 7 into home_column variable, so the cursor can not go below 7
    lda #34                                       // Load 39 into accumulator
    sta LIMIT_COLM                                // Store 39 into the limit_column so the cursor can not go beyond that position
    lda #0                                        // Load 0 - don't clear field so user can edit existing text
    sta CLEAR_FIELD_FLAG                          // SET clear text flag to 0 (keep existing text visible for editing)
    lda #31                                       //
    sta CURSORCOLOR                               //
    jsr !config_text_input+                              // Call the text input routine, we will be back when the user presses RETURN    
    jsr invert_cursor
    lda #15                                        // Load 6 into accumulator
    sta HOME_LINE                                 // Store 6 into Home_line variable, so the cursor van not go above line 22
    sta LIMIT_LINE                                // Store 6 into limit_line variable so the cursor van not go below line 24
    lda #19                                       // Load 11 into accumulator
    sta HOME_COLM                                 // Store 11 into home_column variable, so the cursor can not go below 10
    lda #24                                       // Load 5 into accumulator
    sta LIMIT_COLM                                // Store 5 into the limit_column so the cursor can not go beyond that position
    lda #0                                        // Load 0 - don't clear field so user can edit existing text
    sta CLEAR_FIELD_FLAG                          // 
    lda #149                                      //
    sta CURSORCOLOR                               //
    jsr !config_text_input+                              // Call the text input routine, we will be back when the user presses RETURN
    jsr invert_cursor
    jsr !hide_cursor-                             // Hide the cursor
                                                  // 
    displayText(text_save_settings,17,3)          // Display "[ F1 Save settings" on line 13, row 3 
                                                  // 
!keyinput:                                        // At this point the user can select F1 or F7 to Save settings and Test settings, or exit the menu
                                                  // 
    jsr !wait_for_a_key+                          //
    cmp #133                                      // F1 key pressed?
    beq !save_settings+                           // If true, save the osc settings
    cmp #136                                      // F7 key pressed?
    beq !exit_menu+                               // If true, exit to main menu.
    jmp !keyinput-                                // Ignore all other keys and wait for user input again
                                                  // 
!exit_menu:                                       // F7 Pressed!
    jmp !config_f7_exit+                           // Clean exit to main menu
                                                  // 
invert_cursor:                                    // Global routine to remove inverted character attributes from current field
                                                  // Uses HOME_LINE and HOME_COLM to determine which field to clear
    ldx HOME_LINE                                 // Load the line number (0-24)
    lda screen_lines_low,x                        // Get low byte of screen address for this line
    clc                                           // Clear carry
    adc HOME_COLM                                 // Add HOME_COLM to get start of field
    sta $fc                                       // Store in $fc (low byte of pointer)
    lda screen_lines_high,x                       // Get high byte of screen address for this line
    adc #0                                        // Add carry if any
    sta $fd                                       // Store in $fd (high byte of pointer)
    ldy #0                                        // y will be offset from start of field
!loop:
    tya                                           // Transfer y to accumulator
    clc                                           // Clear carry
    adc HOME_COLM                                 // Add HOME_COLM to get current column
    cmp LIMIT_COLM                                // Compare with limit
    bcs !done+                                    // If >= limit, we're done (LIMIT_COLM is exclusive)
    lda ($fc),y                                   // Load character from screen (using pointer at $fc/$fd)
    and #%01111111                                // Remove inverted bit (bit 7)
    sta ($fc),y                                   // Store back to screen
    iny                                           // Increment offset
    jmp !loop-                                    // Continue loop
!done:
    rts                                           // Return

!save_settings:                                   // Read the ip address from screen into the TXBUFFER
    ldx #$00                       //osc ip1 
    ldy #$00
!loop:
    lda $0503,x              // load a byte from the screen buffer
    cmp #$20 
    beq !+
    sta TXBUFFER,y            // store the byte in the TXBUFFER
    inx                       // increase the x index
    iny                       // increase the y index
    jmp !loop-
!:
    lda #129                 // delimiter byte in buffer
    sta TXBUFFER,y
    iny

    ldx #$00                  //osc port1
!loop:
    lda $0503+40,x
    cmp #$20 
    beq !+
    sta TXBUFFER,y
    inx
    iny
jmp !loop-
!:   
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //osc ip2
!loop:
    lda $0503+80,x
    cmp #$20 
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //osc port2
!loop:
    lda $0503+120,x
    cmp #$20
    beq !+ 
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:   
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //osc ip3
!loop:
    lda $0503+160,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
jmp !loop-

!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00
!loop:
    lda $0503+200,x          //osc port3
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:   
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //osc ip4
!loop:
    lda $0503+240,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //osc port4
!loop:
    lda $0503+280,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:   
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //osc ip5
!loop:
    lda $0503+320,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //osc port5
!loop:
    lda $0503+360,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
 
    lda #128
    sta TXBUFFER,y
    iny
    jsr !wait_for_ready_to_receive+               // Prepare the ESP to receive
    lda #228                                      // Load 228 in accumulator (228 = set OSC server IP and port)
    sta _IO1_                                     // Send the start byte (228)
    jsr !send_buffer+                             // Send the OSC configuration to the ESP32
                                                  // Read the port from screen into the TXBUFFER
                                                  // 
    ldx #22 ; jsr $E9FF                           // Clear line 22
    inx ; jsr $E9FF                               // Clear line 23
    displayText(text_settings_saved,23,6)         // 
                                                  // 
    lda #255                                      // Delay 255... hamsters
    sta DELAY                                     // Store 255 in the DELAY variable
    jsr !delay+                                   // and call the delay subroutine
    jsr !delay+                                   // a few times
    jsr !delay+                                   // a few times
    jsr !delay+                                   // a few times
    jsr !delay+                                   // a few times
    jmp !osc_setup-                           // Rinse and repeat
//=========================================================================================================
//    MENU TASMOTA SETUP SCREEN (F8)
//=========================================================================================================
// send byte 223 to get Tasmota device list (20 fields: name1[129]ip1[129]...name10[129]ip10[129])
// send byte 222 to save Tasmota device list (20 fields: name1[129]ip1[129]...name10[129]ip10[129])
//=========================================================================================================
!tasmota_setup:                                   // 
    lda #1
    sta INCONFIG
    lda #255                                      //
    sta DELAY                                     //
    lda #1
    sta MENU_ID
    jsr !checkWiFi-                               // check if we have wifi   
    jsr !start_menu_screen-                       // 
    displayText(text_tasmota_menu,1,15)           // Display the menu title on line 1, row 15
!ac_wifi_check:                                   //
    lda HAVEWIFI                                  //
    cmp #1                                        //
    beq !+                                        //
    displayText(text_any_key,21,7)                //
    displayText(text_error_no_internet,4,1)       //
    jsr !wait_for_a_key+                          //
    jmp !mainmenu-                                //
!:
    displayText(text_tasmota1_name,4,1)           // Display "Name:" on line 4, row 1
    displayText(text_tasmota1_ip,5,1)             // Display "IP:" on line 5, row 1
    displayText(text_tasmota2_name,6,1)           // Display "Name:" on line 6, row 1
    displayText(text_tasmota2_ip,7,1)             // Display "IP:" on line 7, row 1
    displayText(text_tasmota3_name,8,1)           // Display "Name:" on line 8, row 1
    displayText(text_tasmota3_ip,9,1)             // Display "IP:" on line 9, row 1
    displayText(text_tasmota4_name,10,1)          // Display "Name:" on line 10, row 1
    displayText(text_tasmota4_ip,11,1)            // Display "IP:" on line 11, row 1
    displayText(text_tasmota5_name,12,1)          // Display "Name:" on line 12, row 1
    displayText(text_tasmota5_ip,13,1)            // Display "IP:" on line 13, row 1
    displayText(text_tasmota6_name,14,1)          // Display "Name:" on line 14, row 1
    displayText(text_tasmota6_ip,15,1)            // Display "IP:" on line 15, row 1
    displayText(text_tasmota7_name,16,1)          // Display "Name:" on line 16, row 1
    displayText(text_tasmota7_ip,17,1)            // Display "IP:" on line 17, row 1
    displayText(text_tasmota8_name,18,1)          // Display "Name:" on line 18, row 1
    displayText(text_tasmota8_ip,19,1)            // Display "IP:" on line 19, row 1
    displayText(text_tasmota9_name,20,1)          // Display "Name:" on line 20, row 1
    displayText(text_tasmota9_ip,21,1)            // Display "IP:" on line 21, row 1
    displayText(text_tasmota10_name,22,1)         // Display "Name:" on line 22, row 1
    displayText(text_tasmota10_ip,23,1)           // Display "IP:" on line 23, row 1

    lda #24 ; sta _LINE_POS_                      // Load 24 into accumulator and store it in zero page address $fb
    jsr !draw_menu_line+                          // Call the draw_menu_line sub routine to draw a line on row 24  
                                                  //
    lda VICEMODE                                  // \                                  
    cmp #1                                        //  \ Skip receiving the data from the cartridge if the cartridge is not attachted! 
    bne !+                                        //  /  
    jmp !fill_fields+                             // /   
                                                  // 
!sendcmd:                                         // 
!:  lda #223                                      // load the number #223
    sta CMD                                       // Store that in variable CMD
    jsr !send_start_byte_ff+                      // Call the sub routine to send 223 to the esp32 to ask for the tasmota devices
    lda TIMOUTERROR                               // RXBUFFER now contains name1[129]ip1[129]...name10[129]ip10[129]
    cmp #1                                        //
    bne !+                                        //           
    jsr !delay+                                   //
    jmp !sendcmd-                                 //
                                                  //
!:  lda #145                                      // Load 145 (white color code) into accumulator - set color first
    sta $5c                                       // Initialize text color to white before displaying
    lda #1                                        // we need the first element from the RXBUFFER (device 1 name)
    sta $02                                       // store 1 (1=first element) in $02
    jsr !splitRXbuffer-                           // copy the first element to Splitbuffer
    displayText(SPLITBUFFER,4,7)                  // Display the buffer (containing device 1 name) on screen
                                                  // 
    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #2                                        // now we need the second element from the RX buffer (device 1 ip)
    sta $02                                       // so we put #2 in address $02
    jsr !splitRXbuffer-                           // and call the split routine to copy the element to the Splitbuffer
    displayText(SPLITBUFFER,5,7)                  // Display the buffer (containing device 1 ip) on screen

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #3                                        // device 2 name
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,6,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #4                                        // device 2 ip
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,7,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #5                                        // device 3 name
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,8,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #6                                        // device 3 ip
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,9,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #7                                        // device 4 name
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,10,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #8                                        // device 4 ip
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,11,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #9                                        // device 5 name
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,12,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #10                                       // device 5 ip
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,13,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #11                                       // device 6 name
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,14,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #12                                       // device 6 ip
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,15,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #13                                       // device 7 name
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,16,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #14                                       // device 7 ip
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,17,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #15                                       // device 8 name
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,18,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #16                                       // device 8 ip
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,19,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #17                                       // device 9 name
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,20,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #18                                       // device 9 ip
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,21,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #19                                       // device 10 name
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,22,7)

    lda #145                                      // Load 145 (white color code) into accumulator
    sta $5c                                       // Initialize text color to white before displaying
    lda #20                                       // device 10 ip
    sta $02
    jsr !splitRXbuffer-
    displayText(SPLITBUFFER,23,7)
                                                  // Now we need text input boxes so the user can change the names and IPs
!fill_fields:                                     // Set the limits to where the cursor can travel
    lda #5                                        // Load 5 into accumulator (same as OSC setup)
    sta MENU_ID                                   // and store it as the ID of this menu
                                                  // Device 1 Name field
    lda #4                                        // Load 4 into accumulator
    sta HOME_LINE                                 // Store 4 into Home_line variable
    sta LIMIT_LINE                                // Store 4 into limit_line variable
    lda #7                                        // Load 7 into accumulator
    sta HOME_COLM                                 // Store 7 into home_column variable
    lda #26                                       // Load 26 into accumulator (20 characters: columns 7-26)
    sta LIMIT_COLM                                // Store 26 into the limit_column
    lda #0                                        // Load 0 - don't clear field so user can edit existing text
    sta CLEAR_FIELD_FLAG                          // SET clear text flag to 0
    lda #31                                       //
    sta CURSORCOLOR                               //
    jsr !config_text_input+                              // Call the text input routine
    jsr invert_cursor                             // Remove inverted character attributes
                                                  // Device 1 IP field
    lda #5                                        // Load 5 into accumulator (IP on line 5)
    sta HOME_LINE                                 // Store 5 into Home_line variable
    sta LIMIT_LINE                                // Store 5 into limit_line variable
    lda #7                                        // Load 7 into accumulator
    sta HOME_COLM                                 // Store 7 into home_column variable
    lda #22                                       // Load 22 into accumulator (15 characters: columns 7-21, LIMIT_COLM is exclusive)
    sta LIMIT_COLM                                // Store 22 into the limit_column
    lda #0                                        // Load 0 - don't clear field so user can edit existing text
    sta CLEAR_FIELD_FLAG                          //
    lda #149                                      //
    sta CURSORCOLOR                               //
    jsr !config_text_input+                              // Call the text input routine
    jsr invert_cursor
                                                  // Device 2 Name field
    lda #6                                        // Load 6 into accumulator (Name on line 6)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                        // Load 7 into accumulator
    sta HOME_COLM                                 
    lda #26                                       // Load 26 into accumulator (20 characters: columns 7-26)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #31                                       
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 2 IP field
    lda #7                                        // Load 7 into accumulator (IP on line 7)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                       
    sta HOME_COLM                                 
    lda #22                                       // Load 22 into accumulator (15 characters: columns 7-21, LIMIT_COLM is exclusive)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #149                                      
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 3 Name field
    lda #8                                        // Load 8 into accumulator (Name on line 8)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                        // Load 7 into accumulator
    sta HOME_COLM                                 
    lda #26                                       // Load 26 into accumulator (20 characters: columns 7-26)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #31                                       
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 3 IP field
    lda #9                                        // Load 9 into accumulator (IP on line 9)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                       
    sta HOME_COLM                                 
    lda #22                                       // Load 22 into accumulator (15 characters: columns 7-21, LIMIT_COLM is exclusive)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #149                                      
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 4 Name field
    lda #10                                       // Load 10 into accumulator (Name on line 10)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                        // Load 7 into accumulator
    sta HOME_COLM                                 
    lda #26                                       // Load 26 into accumulator (20 characters: columns 7-26)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #31                                       
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 4 IP field
    lda #11                                       // Load 11 into accumulator (IP on line 11)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                       
    sta HOME_COLM                                 
    lda #22                                       // Load 22 into accumulator (15 characters: columns 7-21, LIMIT_COLM is exclusive)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #149                                      
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 5 Name field
    lda #12                                       // Load 12 into accumulator (Name on line 12)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                        // Load 7 into accumulator
    sta HOME_COLM                                 
    lda #26                                       // Load 26 into accumulator (20 characters: columns 7-26)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #31                                       
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 5 IP field
    lda #13                                       // Load 13 into accumulator (IP on line 13)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                       
    sta HOME_COLM                                 
    lda #22                                       // Load 22 into accumulator (15 characters: columns 7-21, LIMIT_COLM is exclusive)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #149                                      
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 6 Name field
    lda #14                                       // Load 14 into accumulator (Name on line 14)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                        // Load 7 into accumulator
    sta HOME_COLM                                 
    lda #26                                       // Load 26 into accumulator (20 characters: columns 7-26)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #31                                       
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 6 IP field
    lda #15                                       // Load 15 into accumulator (IP on line 15)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                       
    sta HOME_COLM                                 
    lda #22                                       // Load 22 into accumulator (15 characters: columns 7-21, LIMIT_COLM is exclusive)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #149                                      
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 7 Name field
    lda #16                                       // Load 16 into accumulator (Name on line 16)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                        // Load 7 into accumulator
    sta HOME_COLM                                 
    lda #26                                       // Load 26 into accumulator (20 characters: columns 7-26)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #31                                       
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 7 IP field
    lda #17                                       // Load 17 into accumulator (IP on line 17)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                       
    sta HOME_COLM                                 
    lda #22                                       // Load 22 into accumulator (15 characters: columns 7-21, LIMIT_COLM is exclusive)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #149                                      
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 8 Name field
    lda #18                                       // Load 18 into accumulator (Name on line 18)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                        // Load 7 into accumulator
    sta HOME_COLM                                 
    lda #26                                       // Load 26 into accumulator (20 characters: columns 7-26)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #31                                       
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 8 IP field
    lda #19                                       // Load 19 into accumulator (IP on line 19)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                       
    sta HOME_COLM                                 
    lda #22                                       // Load 22 into accumulator (15 characters: columns 7-21, LIMIT_COLM is exclusive)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #149                                      
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 9 Name field
    lda #20                                       // Load 20 into accumulator (Name on line 20)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                        // Load 7 into accumulator
    sta HOME_COLM                                 
    lda #26                                       // Load 26 into accumulator (20 characters: columns 7-26)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #31                                       
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 9 IP field
    lda #21                                       // Load 21 into accumulator (IP on line 21)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                       
    sta HOME_COLM                                 
    lda #22                                       // Load 22 into accumulator (15 characters: columns 7-21, LIMIT_COLM is exclusive)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #149                                      
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 10 Name field
    lda #22                                       // Load 22 into accumulator (Name on line 22)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                        // Load 7 into accumulator
    sta HOME_COLM                                 
    lda #26                                       // Load 26 into accumulator (20 characters: columns 7-26)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #31                                       
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
                                                  // Device 10 IP field
    lda #23                                       // Load 23 into accumulator (IP on line 23)
    sta HOME_LINE                                 
    sta LIMIT_LINE                                
    lda #7                                       
    sta HOME_COLM                                 
    lda #22                                       // Load 22 into accumulator (15 characters: columns 7-21, LIMIT_COLM is exclusive)
    sta LIMIT_COLM                                
    lda #0                                        
    sta CLEAR_FIELD_FLAG                          
    lda #149                                      
    sta CURSORCOLOR                               
    jsr !config_text_input+                              
    jsr invert_cursor
    jsr !hide_cursor-                             // Hide the cursor
                                                  // 
    displayText(text_save_settings,24,3)          // Display "[ F1 Save settings" on line 24, row 3
    displayText(text_exit_menu,24,20)             // Display "[ F7 ] exit menu" on line 24, row 20 (bottom of screen)
                                                  // 
!keyinput:                                        // At this point the user can select F1 or F7 to Save settings or exit the menu
                                                  // 
    jsr !wait_for_a_key+                          //
    cmp #133                                      // F1 key pressed?
    beq !save_settings+                           // If true, save the tasmota settings
    cmp #136                                      // F7 key pressed?
    bne !+                                        // If not F7, skip to next check
    jmp !exit_menu+                               // If true, exit to main menu (long jump)
!:  jmp !keyinput-                                // Ignore all other keys and wait for user input again
                                                  // 
!save_settings:                                   // F1 Pressed! Save all the tasmota device settings
                                                  // Read all 20 fields from screen memory into TXBUFFER
                                                  // Format: name1[129]ip1[129]name2[129]ip2[129]...name10[129]ip10[129]
                                                  // Tasmota base address: Line 4, column 7 = $04A7
                                                  // Use same approach as OSC setup (direct addressing)
    ldx #$00                       //tasmota device 1 name (line 4, col 7 = $04A7)
    ldy #$00
!loop:
    lda $04A7,x              // load a byte from the screen buffer
    cmp #$20 
    beq !+
    sta TXBUFFER,y            // store the byte in the TXBUFFER
    inx                       // increase the x index
    iny                       // increase the y index
    jmp !loop-
!:
    lda #129                 // delimiter byte in buffer
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 1 ip (line 5, col 7 = $04CF = $04A7+40)
!loop:
    lda $04CF,x
    cmp #$20 
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 2 name (line 6, col 7 = $04F7 = $04A7+80)
!loop:
    lda $04F7,x
    cmp #$20 
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 2 ip (line 7, col 7 = $051F = $04A7+120)
!loop:
    lda $051F,x
    cmp #$20
    beq !+ 
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 3 name (line 8, col 7 = $0547 = $04A7+160)
!loop:
    lda $0547,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 3 ip (line 9, col 7 = $056F = $04A7+200)
!loop:
    lda $056F,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 4 name (line 10, col 7 = $0597 = $04A7+240)
!loop:
    lda $0597,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 4 ip (line 11, col 7 = $05BF = $04A7+280)
!loop:
    lda $05BF,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 5 name (line 12, col 7 = $05E7 = $04A7+320)
!loop:
    lda $05E7,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 5 ip (line 13, col 7 = $060F = $04A7+360)
!loop:
    lda $060F,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 6 name (line 14, col 7 = $0637 = $04A7+400)
!loop:
    lda $0637,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 6 ip (line 15, col 7 = $065F = $04A7+440)
!loop:
    lda $065F,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 7 name (line 16, col 7 = $0687 = $04A7+480)
!loop:
    lda $0687,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 7 ip (line 17, col 7 = $06AF = $04A7+520)
!loop:
    lda $06AF,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 8 name (line 18, col 7 = $06D7 = $04A7+560)
!loop:
    lda $06D7,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 8 ip (line 19, col 7 = $06FF = $04A7+600)
!loop:
    lda $06FF,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 9 name (line 20, col 7 = $0727 = $04A7+640)
!loop:
    lda $0727,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 9 ip (line 21, col 7 = $074F = $04A7+680)
!loop:
    lda $074F,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 10 name (line 22, col 7 = $0777 = $04A7+720)
!loop:
    lda $0777,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #129
    sta TXBUFFER,y
    iny

    ldx #$00                  //tasmota device 10 ip (line 23, col 7 = $079F = $04A7+760)
!loop:
    lda $079F,x
    cmp #$20
    beq !+
    sta TXBUFFER,y
    inx
    iny
    jmp !loop-
!:
    lda #128                                      // End delimiter
    sta TXBUFFER,y
                                                  // Send command 222 and buffer - use same method as OSC setup
    jsr !wait_for_ready_to_receive+               // Prepare the ESP to receive
    lda #222                                      // Load 222 in accumulator (save tasmota device list)
    sta _IO1_                                     // Send the start byte (222)
    jsr !send_buffer+                             // Send the new tasmota device list to the ESP32
    
    ldx #22 ; jsr $E9FF                           // Clear line 22
    inx ; jsr $E9FF                               // Clear line 23
    displayText(text_settings_saved,23,6)         // Display "Settings saved"
                                                  // 
    lda #255                                      // Delay 255... hamsters
    sta DELAY                                     // Store 255 in the DELAY variable
    jsr !delay+                                   // and call the delay subroutine
    jsr !delay+                                   // a few times
    jsr !delay+                                   // a few times
    jsr !delay+                                   // a few times
    jsr !delay+                                   // a few times
    jmp !tasmota_setup-                           // Rinse and repeat - jump back to start to reload everything
                                                  // 
!exit_menu:                                       // F7 Pressed!
    jmp !config_f7_exit+                           // Clean exit to main menu
//=========================================================================================================
//    END OF TASMOTA SETUP MENU
//=========================================================================================================    




//=========================================================================================================
//    Function to wait for a key input
//=========================================================================================================
!wait_for_a_key:
!wait_for_any_key:                                //
    jsr $ffe4                                     // wait for any key                               
    beq !wait_for_any_key-                        //
    rts

//=========================================================================================================
//    Config menu F7 exit helpers (avoid pla/pla stack corruption from jmp exit_F7)
//=========================================================================================================
!check_f7_after_text_input:
    lda F7_EXIT_REQUEST
    beq !+
    jmp !config_f7_exit+
!:
    rts

!config_f7_exit:
    lda #0
    sta F7_EXIT_REQUEST
    jsr !hide_cursor-
    lda #0
    sta INCONFIG
    jmp !mainmenu-

!config_text_input:
    jsr !text_input+
    jsr !check_f7_after_text_input-
    rts

//=========================================================================================================
//    Function for text input
//=========================================================================================================
!text_input:                                      // 
                                                  // 
!clearhome:                                       // 
    lda MENU_ID                                   // Load the menu ID
    cmp #5                                        // If menu ID is 5 (OSC setup screen),
    beq !m1+                                      // We do not want to clear the line
    cmp #6                                        // If menu ID is 6 (OSC send screen),
    beq !m1+                                      // We do not want to clear the line
    ldx HOME_LINE                                 // Clear the text input box
                                                  // 
!clear_lines:                                     // Start of clear loop
    jsr $E9FF                                     // Clear line in register x
    cpx LIMIT_LINE                                // Are there more lines to clear?
    beq !all_clean+                               // If not, jmp to !all_clean and exit the loop
    inx                                           // Increase x register
    jmp !clear_lines-                             // Jump back to the start of the loop and clear the next line
                                                  // 
!all_clean:                                       // All lines cleared
                                                  // 
!m1:                                              // 
                                                  // 
!home:                                            //  
    clc                                           // Clear carry so we can SET the cursor position
    ldx HOME_LINE                                 // Select row
    ldy HOME_COLM                                 // Select column
    jsr $fff0                                     // Set cursor
    jsr !show_cursor-                             // Show cursor
    lda CURSORCOLOR                               // Load cursor color 
    jsr $ffd2                                     // Output that petscii code to screen to change the cursor to white                                                  
    jsr !fix_inverted_chars+                      // 
!keyinput:                                        //     
    lda #0                                        // write zero to address $d4
    sta $d4                                       // to disable "quote mode" (special characters for cursor movement and such) 
!:  jsr $ffe4                                     // Call kernal routine: Get character from keyboard buffer
    beq !keyinput-                                // Loop if there is none
    cmp #221                                      // Shift Minus gives a vertical bar, we replace it with underscore
    bne !+                                        // If it is any other key, skip to the next !: marker
    lda #175                                      // Change the character into an underscore
!:  jsr !HotKeys+
!:      jsr !preventGraphChars+
    cmp #134                                      // F3 key pressed?
    bne !+                                        // No, try the next possible match
    jmp !exit_F3+                                 // Yes, jump to exit F3
!:  cmp #136                                      // F7 key pressed?
    bne !+                                        // No, try the next possible match
    lda #1
    sta F7_EXIT_REQUEST
    rts
!:  cmp #19                                       // Home key pressed?
    bne !skip_home+                               // No, skip home handler
    jmp !home-                                    // Yes, jump to !home
!skip_home:
    cmp #147                                      // Clear home key pressed?
    bne !+                                        // Yes, jump to !clearhome
    jmp !clearhome-                               // 
!:  cmp #148                                      // Insert key pressed?
    bne !+                                        // 
    jmp !keyinput-                                // Yes, jump to !keyinput
!:  cmp #20                                       // Del key pressed?
    bne !+                                        // 
    jmp !preventleft+                             // Yes, jump to !preventleft
!:  cmp #13                                       // Return key pressed?
    bne !+                                        // 
    jmp !preventdown+                             // Yes, jump to !preventdown
!:  cmp #145                                      // Cursor up pressed?
    bne !+                                        // 
    jmp !preventup+                               // Yes, jump to !preventup
!:  cmp #17                                       // Cursor down pressed?
    bne !+                                        // 
    jmp !preventdown+                             // Yes, jump to !preventdown
!:  cmp #157                                      // Cursor left pressed?
    bne !+                                        // 
    jmp !preventleft+                             // Yes, jump to !preventleft
!:  jmp !preventright+                            // Jump to !preventright
                                                  // 
!keyout:                                          // 
    jsr $ffd2                                     // Output the character to screen    
    jsr !fix_inverted_chars+
    lda $0286                                     // if the current color is black, reset it to white.
    cmp #0                                        // we do not want black text on black background
    bne !+                                        // 
    lda #5                                        // Load 5 in accumulator (petscii code for color white)
    jsr $ffd2                                     // Output that petscii code to screen to change the cursor to white
!:  jmp !keyinput-                                // jump back to key input
                                                  // 
!preventleft:                                     // 
    ldy $d3                                       // $d3 always contains the current column of the cursors position
    cpy HOME_COLM                                 // 
    beq !preventup+                               // 
    jmp !keyout-                                  // 
                                                  // 
!preventup:                                       // 
    ldx $d6                                       // $d6 alway contains the current line of the cursor position
    cpx HOME_LINE                                 // 
    beq !exit+                                    // 
    jmp !keyout-                                  // 
                                                  // 
!preventright:                                    // Prevent right is always in the loop, because normal typing can also cause you to go out of boundries!
!:  pha                                           // push the accu to the stack to keep it save
    jsr !clear_field+                             // We call !clear_field here because Prevent right is always in the loop.    
    pla                                           // restore the accumulator
    ldy $d3                                       // $d3 always contains the current column of the cursors position
    cpy LIMIT_COLM                                // 
    beq !preventdown+                             // 
    jmp !keyout-                                  // 
                                                  // 
!preventdown:                                     // 
    cmp #13                                       // Find out if we are here because the return key was pressed
    bne !notReturn+                               // 
    ldx $d6                                       // $d6 alway contains the current line of the cursor position
    cpx LIMIT_LINE                                // 
    bne !+                                        // 
    jsr !fix_inverted_chars+                      //
    rts                                           // Return to caller, this exits the keyinput routine!        
!:  clc                                           // clear carry bit so we can SET the cursor position
    inx                                           // x has the line number, so increase that                                     
    ldy #0                                        // Select column to zero (start of the line)
    jsr $fff0                                     // call the set cursor kernal routine 
    jmp !exit+                                    // and jump to exit
!notReturn:                                       // we are NOT here because user pressed return (maybe user pressed cursor down)
    ldx $d6                                       // $d6 alway contains the current line of the cursor position
    cpx LIMIT_LINE                                // se if we are on the final line
    beq !exit+                                    // if so, ignore this key press, jump to exit
    jmp !keyout-                                  // if not, no problem, output the key
                                                  // 
!exit:                                            //             
    lda #0                                        // load zero in accumulator (delete the keystroke that was in there)
    jmp !keyout-                                  // and jump to keyout
                                                  // 
!exit_F3:                                         // Exit OSC edit mode (F3)
    lda MENU_ID                                   // Check if we're in OSC send/edit mode
    cmp #6                                        // MENU_ID 6 = OSC send screen
    beq !exit_osc_edit+                           // If yes, exit edit mode back to cuelist
    jmp !exit-
!exit_osc_edit:                                   // Exit OSC edit mode back to cuelist
    jmp !main_osc_screen-                         // Jump back to OSC send screen (cuelist mode)
                                                  //
!exit_F7:
    jmp !config_f7_exit-

!HotKeys:
!:  tax                                           // store the accumulator in x temp. (transfer a to x)
    lda $28d                                      // read state of special keys, 4=Control key
    cmp #4                                        // is controll pressed
    bne !+                                        // if so branch to !+
    lda #1
    sta TXBUFFER +2
    jmp !c+
!:  cmp #2    
    bne !+    
    lda #5
    sta TXBUFFER +2
    jmp !c+
!:  txa                                           // if not, restore the accumulator (transfer x to a)
    rts                                           // and return
!c: lda #%11101111 // select column 4 (bit 4 = 0)
    sta $dc00
    lda $dc01
    and #%00010000 // row 4
    bne !skeys+      
    jsr handle_mute 
    txa
    rts
!skeys:
    txa
    rts

!preventGraphChars:                               //
    cmp #175                                      // allow underscore, move to the exit
    beq !exit+                                    //
    cmp #94                                       // ignore arrow up
    beq !ignore+                                  //
    cmp #95                                       // ignore arrow to left
    beq !ignore+                                  //
    cmp #219                                      // ignore everything above 219
    bcs !ignore+                                  //
    cmp #160                                      // ignore every thing between 160 and 192 
    bcc !exit+                                    //
    cmp #192                                      //
    bcc !ignore+                                  //
!exit:                                            //
    rts                                           //
!ignore:                                          //
    lda #0                                        //
    rts                                           //

handle_mute:
    lda MUTE
    eor #1  // toggle 0 and 1     
    sta MUTE  
display_mute_status:
    lda MUTE
    cmp #1
    bne !nm+  
    displayText(text_mute,21,1)
    jmp !+
!nm: displayText(text_not_mute,21,1) 
!:  rts

//=========================================================================================================
// SUB ROUTINE, CHECK CONFIGURATION STATUS
//=========================================================================================================
!callstatus:                                      // 
    lda VICEMODE                                  // if we are running in simulation mode
    cmp #1                                        // jump to exit without interacting with ESP32
    bne !+                                        // branche if not equal (not in vice mode) to the next label
    lda #4                                        // if we are in vice mode, we load 4 into the rxbuffer (4 means fully configured, 5 means empty configuration)
    sta CONFIG_STATUS                             // so the program thinks we have a complete configuration
    ldx 0                                         //
                                                  //
    jmp !exit+                                    // exit this routine, we are in vice mode at this point
                                                  // 
!:  lda #236                                      // Load 236 in accumulator (get current connection status and servername)
    sta CMD                                       // Store that in CMD
    jsr !send_start_byte_ff+                      // Call the sub routine to obtain connection status from esp32
                                                  // the RXBUFFER now contains config_status[32]servername[128]
                                                  //
                                                  //
    lda #1                                        // set the variables up for the splitbuffer command
    sta $02                                       // we need the first element, so store #1 in $02
    jsr !splitRXbuffer-                           // and call spilt buffer
    lda SPLITBUFFER                               // SPLITBUFFER NOW CONTAINS THE CONFIG_STATUS
    sta CONFIG_STATUS                             // this is only one character, store it in config_status   
!exit:                                            // 
    rts                                           // and return to caller
                                                  // 
//=========================================================================================================
// SUB ROUTINE, SEND CONFIGURATION STATUS
//=========================================================================================================
!sendstatus:                                      //
    ldy VICEMODE                                  // if we are running in simulation mode
    cpy #1                                        // jump to exit without interacting with ESP32
    bne !+                                        // 
    jmp !exit+                                    // 
                                                  // 
!:  ldy #0                                        // Y is our register, used for the index
    sta TXBUFFER,y                                // Put status character in the buffer at index y
    sta CONFIG_STATUS                             //
    iny                                           // Increase index
    lda #128                                      // Load 128 in accumulator
    sta TXBUFFER,y                                // Store 128 in the buffer to finish the buffer
    jsr !wait_for_ready_to_receive+               // Prepare the ESP to receive
    lda #235                                      // Load 235 into accumulator
    sta _IO1_                                     // Send the start byte (235 = configuration status)
    jsr !send_buffer+                             // 
!exit:                                            // 
    rts                                           // Return to caller
                                                  //
//=========================================================================================================
// SUB ROUTINE, CLEAR FIELD
//=========================================================================================================
!clear_field:                                     // 
                                                  // 
    lda CLEAR_FIELD_FLAG                          // Load the Clear flag
    cmp #1                                        // Compare it to #1
    beq !do_clear_field+                          // If it is #1, clear the rest of the fied
    rts                                           // If not return to caller
!do_clear_field:                                  // 
    dec CLEAR_FIELD_FLAG                          // First set clear flag back to zero
                                                  // 
                                                  // How to clear the field?
                                                  // We know that the cursor is on coordinated HOME_LINE, HOME_COLM
                                                  // So we print space characters until we reach LIMIT_COLM
                                                  // Then set the cursor back at the coordinates HOME_LINE,HOMECOLM
                                                  // 
    ldy HOME_COLM                                 // 
!loop:                                            // Now we loop y until LIMIT_COL is reached
    lda #32                                       // Load screen code for SPACE character
    jsr $ffd2                                     // Output the character to screen
    iny                                           // Increase y
    cpy LIMIT_COLM                                // Compare Y with limit_colm
    bne !loop-                                    // Continue the loop if not equal
    clc                                           // Clear the carry flag so we can SET the cursor back
    ldx HOME_LINE                                 // KERNAL routine $fff0 uses x,y for line,column. So HOME_LINE goes into x
    ldy HOME_COLM                                 // Load the home_colm into y register
    jsr $fff0                                     // Set cursor back to home_colm , x=line, y=column
    rts                                           // Return to caller
                                                  // 
//=========================================================================================================
// SUB ROUTINE, READ TEXT FIELD FROM SCREEN
//=========================================================================================================
!read_from_screen:                                // 
                                                  // 
    ldy #0                                        // y will be used as index for the text
!read_character:                                  // 
                                                  // we have a pointer in zero page address $fb that points to the screen position
    lda (_FIELD_),y                               // Read a character from screen, start at address ($fb), y is the offset
    sta TXBUFFER,y                                // Put the character in the buffer at index y
    iny                                           // Increment y
    cpy READLIMIT                                 // If y has reached the readlimit, the text field is finished
    bne !read_character-                          // If not jump back to read another character
                                                  // Close the buffer
    dey                                           // Decrement y
    lda #128                                      // Load 128 in accumulator
    sta TXBUFFER,y                                // Store 128 in the buffer to finish the buffer
    rts                                           // Return to sender, just like elvis.
                                                  // 
//=========================================================================================================
// SUB ROUTINE, START SCREEN
//=========================================================================================================
!start_screen:                                    // 
                                                  // 
    lda #21                                       // switch to UPPER CASE/PETSCII MODE
    sta $D018                                     // 
    ldx #0                                        // black screen and border
    stx $d021                                     // 
    stx $d020                                     // 
    ldx #0                                        // 
!line:  
    lda introscreen_colors,x
    sta $d800,x
    lda introscreen_colors+$100,x
    sta $d900,x
    lda introscreen_colors+$200,x
    sta $da00,x
    lda introscreen_colors+$300,x
    sta $db00,x
    lda introscreen_chars,x
    sta $0400,x
    lda introscreen_chars+$100,x
    sta $0500,x
    lda introscreen_chars+$200,x
    sta $0600,x
    lda introscreen_chars+$300,x
    sta $0700,x
    inx
    bne !line-                                          // in this loop we draw line characters on the screen, starting in two places on the screen
                                                  // 

    jsr !callstatus-                              // Check the configuration status
!:  jsr $ffe4                                     // get character from keyboard buffer
    beq !-                           // loop if there is none
!:  lda #5                                        // 
    sta CURSORCOLOR                               // set current color to 1 (white)
    rts                                           // Return to caller

//=========================================================================================================
//  SUB ROUTINE DISPLAY TEXT
//=========================================================================================================
!displaytextK:                                    // 
                                                  // first we find out if the text needs to be inverted
                                                  // if the first byte in the text is 143, we will invert the text
                                                  // Inverting the text is done by adding, or bitwise OR, with the number 128
                                                  // see the ora $4b command further down
                                                  // 
    lda #0                                        // by default INVERT = 0 so invertion does not work
    sta INVERT                                    // 
    ldy #0                                        // start index into text buffer
    lda (_BUFFER_POINTER_),y                      // load the very first character of the text
    cmp #143                                      // if it is not equal to 143, do nothing, skip to the next !: label
    bne !+                                        // 
    lda #128                                      // if the text starts with 143, load the number 128
    sta INVERT                                    // in to INVERT
    iny                                           // skip invert prefix byte
                                                  // 
!:                                                // $fb $fc = pointer to the text
    sty $f6                                       // save start index for buffer reads
    ldx _SCREENLINE_                              // zero page f7 has the line number where the text should be displayed
    lda screen_lines_low,x                        // we need to create a pointer in $c1 $c2 to the location in screen RAM
    sta $c1                                       // and a pointer in $c3 $c4 to the location in color RAM
    sta $c3                                       // the lower byte of the color ram is the same as the screen ram
    lda screen_lines_high,x                       // get the high byte for the screen ram
    sta $c2                                       // store it in $c2 to complete the pointer
    lda color_lines_high,x                        // 
    sta $c4                                       // we now have pointers to the line, we need to add the column to end up in the exact address
                                                  // 
    clc                                           // Clear the carry flag, we are going to do some additions (adc) so we need to clear the flag
    lda $c3                                       // load the low byte of the pointer to color RAM (the pointer is in $c3,$c4)
    adc _SCREENCOLM_                              // add the column number (stored in $f8)
    sta $c3                                       // put the result back in $c3 (the low byte for the screen RAM pointer)
    sta $c1                                       // Also put the same value in $c1 (the low byte for the color RAM pointer)
    bcc !setup_index+                             // if the result was bigger than #$FF (#255) then the carry flag is set and we need to increase the high byte of the pointer also
    inc $c4                                       // increase the high byte of the screen RAM pointer with one
    inc $c2                                       // and also the high byte of the color RAM pointer
                                                  // 
!setup_index:                                     // 
    ldy $f6                                       // load start index into y
    sty $ff                                       // we need two indexes, one for reading the buffer
    ldy #0                                        // 
    sty $fe                                       // and one for writing to the screen and color RAM
                                                  // we can not use one index because the buffer may contain bytes for changing the color (144 = black, 145=white, etc)
!readbuffer:                                      // 
    ldy $ff                                       // load the buffer index from address $ff
    lda (_BUFFER_POINTER_),y                      // load a character from the text with y as index this is Indirect-indexed addressing, $fb-$fc contains a pointer to the real address
    cmp #128                                      // compare it to 128, that is the end marker of the text we want to display
    beq !exit+                                    // if equal, exit the loop
    cmp #213                                      // code for skip a number of positions (next byte has that number)
    bne !+                                        //
    inc $ff                                       // set the index to the next byte in the buffer
    iny                                           // increase y
    clc                                           // clear the carry bit, we need to do some a additions (adc)
    lda (_BUFFER_POINTER_),y                      // load the next byte (= number of positions to skip)
    adc $c1                                       // add that to the low byte of the screen memory
    sta $c1                                       // store the result in the low byte of the screen memory
    sta $c3                                       // also store the result in the low byte of the color ram 
    bcc !skip+                                    // see if we rolled over to zero, 
    inc $c2                                       // increase the high bytes in that case
    inc $c4                                       // if not, skip
!skip:                                            //
    inc $ff                                       //
    jmp !readbuffer-                              //
!:  inc $ff                                       // increase the buffer index
    cmp #144                                      // if the byte is 144 or higher, it is not a character but a color code
    bcc !+                                        // if not skip to the next !: label
    sta $5c                                       // store the color code in this address
    jmp !readbuffer-                              // and jump back to read the next byte from the text/buffer
                                                  // 
!:  ldy $fe                                       // load the screen index into y
    ora INVERT                                    // do a bitwise OR operation with the number in address $4b. If the number is 0 nothing will happen. If the number is 128 the character will invert!
    sta ($c1),y                                   // write the character, $c1-$c2 contains a pointer to the address of screen RAM, y is the offset
    lda $5c                                       // load the current color from $5c. this adres contains the current color
    sta ($c3),y                                   // change the color of the character, $c3-$c4 contains a pointer an address in color RAM, y is the offset
    inc $fe                                       // increase the screen index
    jmp !readbuffer-                              // jump back to the beginning of the loop to read the next byte from the text/buffer
                                                  // 
!exit:                                            // at this point we encountered byte 128 in out text string, so we escaped the loop
    rts                                           // return to sender ;-)



//=========================================================================================================
// SUB ROUTINE, DELAY
//=========================================================================================================
!delay:                                           // the delay sub routine is just a loop inside a loop
                                                  // 
    ldx #00                                       // the inner loop counts up to 255
                                                  // 
!loop:                                            // the outer loop repeats that 255 times
                                                  // 
    cpx DELAY                                     // 
    beq !enddelay+                                // 
    inx                                           // 
    ldy #00                                       // 
                                                  // 
!delay:                                           // 
                                                  // 
    cpy #255                                      // 
    beq !loop-                                    // 
    nop                                           // 
    nop                                           // 
    iny                                           // 
    jmp !delay-                                   // 
                                                  // 
!enddelay:                                        // 
    rts                                           // 
                                                  // 
!delay255:                                        //                                                  
  lda #255
  sta DELAY
  jsr !delay-                                                    
  rts
  
!delay100:                                        //                                                  
  lda #100
  sta DELAY
  jsr !delay-                                                    
  rts

//=========================================================================================================
// SUB ROUTINE, diaplay [ F7 ] Exit menu
//=========================================================================================================
!display_F7_menuItem:
   // we use this same line multiple times in the menus and the displayText macro is quite heavy
   // so it makes sense to use a soub routine for this
   displayText(text_exit_menu,17,3)                                                  
   rts                                                   

//=========================================================================================================
// SUB ROUTINE, FIX INVERTED CHARACTERS IN THE MESSAGE LINES
//=========================================================================================================
!fix_inverted_chars:  
   ldx #0
!loop:   
   lda $4A0,x    
   cmp #128
   bcc !+ 
   adc #127
   sta $4A0,x
!: inx
   cpx #250
   beq !exit-
   jmp !loop-
!exit:
   rts
   
   
   
//=========================================================================================================
// SUB ROUTINE, WAIT FOR READY TO RECIEVE SIGNAL FROM ESP32
//=========================================================================================================
!wait_for_ready_to_receive:                       // wait for ready to receive before we send a byte
                                                  // 
    lda _IO2_                                     // read a value from IO2
    cmp #128                                      // compare with 128
    bcc !wait_for_ready_to_receive-               // if smaller try again
    rts                                           // 
                                                  // 
//=========================================================================================================
// SUB ROUTINE, SEND TX BUFFER TO ESP32
//=========================================================================================================
!send_buffer:                                     // 
    lda VICEMODE                                  // if we are running in simulation mode
    cmp #1                                        // jump to exit without interacting with ESP32
    bne !+                                        // 
    jmp !exit+                                    //  
!:  ldx #0                                        // x will be our index when we loop over the RXBUFFER
!sendms:                                          //  
    jsr !wait_for_ready_to_receive-               // wait for ready to receive (bit D7 goes high)
    lda TXBUFFER,x                                // load a byte from the TXBUFFER with index x
    sta _IO1_                                     // send it to IO1
    cmp #128                                      // if the last byte was 128, the buffer is finished
    beq !exit+                                    // exit in that case
    inx                                           // increase the x index
    jmp !sendms-                                  // jump back to send the next byte
                                                  // 
!exit:                                            // 
    rts                                           // 
                                                  // 
//=========================================================================================================
// SUB ROUTINES, to draw horizontal lines on screen (used in the menus)
//=========================================================================================================
!draw_top_menu_lines:                             // this first routine uses the second routine to draw
    lda #0                                        // a line on line 0 and line 2
    sta _LINE_POS_                                // 
    jsr !draw_menu_line+                          // 
    lda #2                                        // 
    sta _LINE_POS_                                // 
    jsr !draw_menu_line+                          // 
    rts                                           // 
//=========================================================================================================
!draw_menu_line:                                  // 
    ldx _LINE_POS_                                // load the desired position of the line from $fb
    lda screen_lines_low,x                        // look up the corresponding address in screen RAM (low byte first)
    sta _LINE_POS_                                // store that in $fb to create a pointer to screen RAM
    sta _LINE_POS_ +2                             // store the same low byte to create a pointer to the corresponding address in color RAM
    lda screen_lines_high,x                       // load the high byte of the address of screen RAM
    sta _LINE_POS_ +1                             // store that in $fc, now we have a pointer $fb,$fc to the screen RAM
    lda color_lines_high,x                        // load the high byte of the address of color RAM
    sta _LINE_POS_ +3                             // store that in $fe, now we have a pointer $fd,$fe to the color RAM
    ldy #0                                        // y is the itterator for the loop
!loop:                                            // start the loop
    lda #64                                       // load the screen code for a horizontal bar
    sta (_LINE_POS_),y                            // put it on screen
    lda _LINE_COLOR_                              // load the color value
    sta (_LINE_POS_ + 2),y                        // put it on screen
    iny                                           // increase y
    cpy #40                                       // if y reaches 40, exit the loop
    bne !loop-                                    // else, continue the loop
    rts                                           // return from subroutine
                                                  // 
//=========================================================================================================
// SUB ROUTINE, send start byte, byte should be stored in CMD
//=========================================================================================================
!send_start_byte_ff:                              // 
    lda VICEMODE                                  // if we are running in simulation mode
    cmp #1                                        // jump to exit without interacting with ESP32
    bne !+                                        // 
    jmp !vicemode+                                //   
                                                  //
!:  lda #0                                        // load zero into accumulator
    sta TIMOUTERROR                               //
    sta RXINDEX                                   // reset the receive buffer index
    sta RXFULL                                    // reset the rxfull flag
    lda #128                                      //
    sta RXBUFFER                                  //
    jsr !wait_for_ready_to_receive-               // 
    lda CMD                                       // load the byte from variable CMD
    sta _IO1_                                     // write the byte to IO1
    cmp #229                                      // for command 229 we send extra info                                               
    bne !+                                        // from the TX Buffer
    jsr !send_buffer-                             // Send extra info to the ESP32 
!:  lda #0                                        // 
    sta TIMEOUT1                                  // we need a simple timeout on this next loop
    sta TIMEOUT2                                  //
!wait_message_complete:                           // wait for a response
    inc TIMEOUT1                                  // increase variable timeout1
    lda TIMEOUT1                                  // 
    cmp #0                                        //
    bne !+                                        //
    inc TIMEOUT2                                  // increase timout2 when timeout1 overloops
    lda TIMEOUT2                                  //
    cmp TIMEOUTVALUE                              // when timeout2 reaches a certain number, exit the loop
    beq !exittimeout+                             //
!:                                                //
    lda RXFULL                                    // load RXFULL flag
    cmp #0                                        // compare with zero
    beq !wait_message_complete-                   // stay in this loop until we get a response
!exit:                                            // 
    rts                                           // return
                                                  // 
!exittimeout:                                     //
    lda #2                                        // make the border red
    sta $d020                                     // for a short while
    lda #30 ; sta DELAY ; jsr !delay-             //
    lda #0                                        // load zero into accumulator
    sta $d020                                     // make the border black again
    sta RXINDEX                                   // reset the receive buffer index
    sta RXFULL                                    // reset the rxfull flag
    lda #128                                      // 
    sta RXBUFFER                                  // Empty and close the RXBUFFER
    sta SPLITBUFFER                               // Empty and close the SPLITBUFFER
    lda #1                                        // set Timeouterror variable to 1
    sta TIMOUTERROR                               //
!vicemode:
    rts                                           //
                                                  //

//=========================================================================================================
// NMI ROUTINE
//=========================================================================================================
nmi:                                              // When the ESP32 loads a byte in the 74ls244 it pulls the NMI line low
                                                  // to signal the C64. Telling it to read the byte
    pushreg()                                     // 
    lda _IO2_                                     // read from IO2. This causes the IO2 line on the cartridge port to go low. Now the ESP32 knows the byte has been received.
    ldx RXINDEX                                   // Load the buffer index into x
    sta RXBUFFER,x                                // write the byte into the buffer index at position x
    cmp #128                                      // a message is complete when we receive 128
    beq !message_complete+                        // jump to then label "message complete" when the message is complete
    inx                                           // increase the x value
    stx RXINDEX                                   // store new x value in RXINDEX
    jmp  !exit_nmi+                               // jump to the exit of this routine
                                                  // 
!message_complete:                                // 
                                                  // 
    lda RXINDEX                                   // load the value of RXINDEX to see how much we have in the buffer
    cmp #0                                        // if the index is still 0, the buffer is empty, set RXFULL to 2 in that case
    bne !not_empty+                               // jump to the next label if the buffer is NOT empty
    lda #2                                        // RXFULL=2 means there is no message in buffer
    sta RXFULL                                    // store #2 in the RXFULL indicator
    jmp  !exit_nmi+                               // and exit the routine
                                                  // 
!not_empty:                                       // if the message is not empty
                                                  // 
    lda #1                                        // Store #1 in the RXFULL indicator
    sta RXFULL                                    // 
                                                  // 
!exit_nmi:                                        // 
    lda #$01                                      // acknowledge the nmi interrupt
    sta $dd0d                                     // you MUST write and read this address to acknowledge the nmi interrupt
    lda $dd0d                                     // you MUST write and read this address to acknowledge the nmi interrupt
    popreg()                                      // 
    rti                                           // return interupt
                                                  // 

    

//=========================================================================================================
// CONSTANTS
//=========================================================================================================              
//fakeline:             .byte 149; .text "Connected, IP Address: 192.168.1.64"; .byte 128
text_menu_item_7_buskmode:             .byte 147; .text "[ F7 ] Busk mode"; .byte 128
text_main_menu:               .byte 151; .text "* OSC64 *"; .byte 128
text_menu_item_1:             .byte 147; .text "[ F1 ] Wifi Setup"; .byte 128
text_menu_item_5_osc:         .byte 147; .text "[ F5 ] Cue List mode";.byte 128
text_menu_item_6_osc_setup:   .byte 147; .text "[ F3 ] OSC Setup";.byte 128
text_menu_item_8_tasmota:     .byte 147; .text "[ F4 ] Tasmota Setup";.byte 128
version:                      .byte 151; .text "3.87"; .byte 128
text_wifi_menu:               .byte 151; .text "WIFI SETUP"; .byte 128
text_wifi_ssid:               .byte 145; .text "SSID:"; .byte 128
text_wifi_password:           .byte 145; .text "Password:"; .byte 128
text_wifi_wait:               .byte 145; .text "Wait for connection (" 
text_wifi_wait_countdown:     .text "9)"; .byte 128
text_save_settings:           .byte 147; .text "[ F1 ] Save"; .byte 128
text_exit_menu:               .byte 147; .text "[ F7 ] Exit"; .byte 128

text_osc_send_menu:           .byte 151; .text "OSC SEND"; .byte 128
text_osc1_ip:                 .byte 145; .text "OSC1 IP:"; .byte 128
text_osc1_port:               .byte 145; .text "OSC1 Port:"; .byte 128
text_osc2_ip:                 .byte 145; .text "OSC2 IP:"; .byte 128
text_osc2_port:               .byte 145; .text "OSC2 Port:"; .byte 128
text_osc3_ip:                 .byte 145; .text "OSC3 IP:"; .byte 128
text_osc3_port:               .byte 145; .text "OSC3 Port:"; .byte 128
text_osc4_ip:                 .byte 145; .text "OSC4 IP:"; .byte 128
text_osc4_port:               .byte 145; .text "OSC4 Port:"; .byte 128
text_osc5_ip:                 .byte 145; .text "OSC5 IP:"; .byte 128
text_osc5_port:               .byte 145; .text "OSC5 Port:"; .byte 128

text_tasmota_menu:            .byte 151; .text "TASMOTA SETUP"; .byte 128
text_tasmota1_name:           .byte 145; .text "Name:"; .byte 128
text_tasmota1_ip:             .byte 145; .text "IP:"; .byte 128
text_tasmota2_name:           .byte 145; .text "Name:"; .byte 128
text_tasmota2_ip:             .byte 145; .text "IP:"; .byte 128
text_tasmota3_name:           .byte 145; .text "Name:"; .byte 128
text_tasmota3_ip:             .byte 145; .text "IP:"; .byte 128
text_tasmota4_name:           .byte 145; .text "Name:"; .byte 128
text_tasmota4_ip:             .byte 145; .text "IP:"; .byte 128
text_tasmota5_name:           .byte 145; .text "Name:"; .byte 128
text_tasmota5_ip:             .byte 145; .text "IP:"; .byte 128
text_tasmota6_name:           .byte 145; .text "Name:"; .byte 128
text_tasmota6_ip:             .byte 145; .text "IP:"; .byte 128
text_tasmota7_name:           .byte 145; .text "Name:"; .byte 128
text_tasmota7_ip:             .byte 145; .text "IP:"; .byte 128
text_tasmota8_name:           .byte 145; .text "Name:"; .byte 128
text_tasmota8_ip:             .byte 145; .text "IP:"; .byte 128
text_tasmota9_name:           .byte 145; .text "Name:"; .byte 128
text_tasmota9_ip:             .byte 145; .text "IP:"; .byte 128
text_tasmota10_name:          .byte 145; .text "Name:"; .byte 128
text_tasmota10_ip:            .byte 145; .text "IP:"; .byte 128

text_mute:     .byte 146; .text "[MUTE]" ; .byte 128
text_not_mute: .byte 156,64,64,64,64,64,64,128 
 

text_settings_saved:          .byte 157; .text "Settings Saved, please wait"; .byte 128
text_error_no_internet:       .byte 146; .text "There is no Internet connection,        go back and check your WiFi Settings";.byte 128
text_any_key:                 .byte 151; .text "Press any key to continue"; .byte 128
text_time_offset:             .byte 145; .text "Time offset from GMT:"; .byte 128

screen_lines_low:             .byte $00,$28,$50,$78,$A0,$C8,$F0,$18,$40,$68,$90,$b8,$e0,$08,$30,$58,$80,$a8,$d0,$f8,$20,$48,$70,$98,$c0 // lookup table
screen_lines_high:            .byte $04,$04,$04,$04,$04,$04,$04,$05,$05,$05,$05,$05,$05,$06,$06,$06,$06,$06,$06,$06,$07,$07,$07,$07,$07 // lookup table
color_lines_high:             .byte $d8,$d8,$d8,$d8,$d8,$d8,$d8,$d9,$d9,$d9,$d9,$d9,$d9,$da,$da,$da,$da,$da,$da,$da,$db,$db,$db,$db,$db // lookup table

song2:     .byte 20,36,20,48,255     
text_osc_cuelist:

                                 .text "           Cue list run mode            "   //0400
                                 .text "                                        "   //0428                                 
                                 .text "F1 Toggle between Busk and Cue list mode"   //0450
                                 .text "F3 Toggle between edit and run mode     "   //0478                                                                  
                                 .text "[RETURN] triggers armed (Yellow) cue    "   //04a0
                                 .text "Up and down arrow move around the list  "   //04c8
                                 .text "                                        "   //04f0                                 
                                 .text "             Active Cue:                "   //0518                                                                                                                                    
                                 .text "                                        "   //0540                                 
                                 .text "                                        "   //0568                                 
                                 .text "                                        "   //0590                                 
                                 .text "                                        "   //05b8                                                                  
                                 .text "             Armed Cue:                 "   //05e0                                 
                                 .text "                                        "   //0608                                 
                                 .text "                                        "   //0630                                                                                                                                    
                                 .text "                                        "   //0658                                 
                                 .text "                                        "   //0680                                 
                                 .text "             Next Cue:                  "   //06a8                                 
                                 .text "                                        "   //06d0                                                                                                                                    
                                 .text "                                        "   //06f8                                 
                                 .text "                                        "   //0720                                 

text_osc_editlist:

                                 .text "          Cue list edit mode            "   //0400 00
                                 .text "                                        "   //0428 01                                
                                 .text "OSC commands can be 80 characters max   "   //0450 02
                                 .text "and can be just a string or hold integer"   //0478 03                                                                 
                                 .text "float or boolean values                 "   //04a0 04
                                 .text "Up and down arrow move around the list  "   //04c8 05
                                 .text "Enter to store                          "   //04f0 06                                
                                 .text "F3 to return to run mode                "   //0518 07                                                                                                                                   
                                 .text "                                        "   //0540 08                                
                                 .text "                                        "   //0568 09                                
                                 .text "                                        "   //0590 10                                
                                 .text "                                        "   //05b8 11                                                                 
                                 .text "              Edit Cue:                 "   //05e0 12                                
                                 .text "                                        "   //0608 13                                
                                 .text "                                        "   //0630 14                                                                                                                                   
                                 .text "                                        "   //0658 15                                
                                 .text "                                        "   //0680 16                                
                                 .text "                                        "   //06a8 17                                
                                 .text "                                        "   //06d0 18                                                                                                                                   
                                 .text "                                        "   //06f8 19                                
                                 .text "                                        "   //0720 20                                


text_osc_buskscreen:             
                                 .text "              Busk mode                 "
                                 .text "                                        "                                    
                                 .text "Keys A through Z and 1 through 0 + space"   
                                 .text "send /c64/key/[Key]                     "                                                                     
                                 .text "Shift + key sends /c64/key/shift/[Key]  "   
                                 .text "                                        "   
                                 .text "                                        "                                                                     
                                 .text "                                        "                                    
                                 .text "          Last sent message:            "                                                                                                                                       
                                 .text "                                        "         
//=========================================================================================================
// VARIABLE BUFFERS
//=========================================================================================================

//=========================================================================================================
// MACROS
//=========================================================================================================
.macro playsong(song){
  lda #<(song)
  sta _NOTE_
  lda #>(song)
  sta _NOTE_ +1
  jsr !playsongk-
  }
  
  
.macro displayText(text,line,column){             // 
                                                  // $f7 = line number
                                                  // $f8 = column
                                                  // $fb $fc = pointer to the text
                                                  // 
    lda #line                                     // 
    sta _SCREENLINE_                              // store the line in zero page address $f7
    lda #column                                   // 
    sta _SCREENCOLM_                              // store the column in zero page address $f8
    lda #<(text)                                  // store the lowbyte of the text location in zero page address $fb
    sta _BUFFER_POINTER_                          // $fb is a zero page address
    lda #>(text)                                  // store the highbyte of the text location in $fc
    sta _BUFFER_POINTER_ +1                       // $FC is is a zero page address
    jsr !displaytextK-                            // Call the displaytext routine
                                                  // 
    }                                             // 
                                                  // 
.macro pushreg(){                                 // 
                                                  // 
    php                                           // push the status register to stack
    pha                                           // push A to stack
    txa                                           // move x to a
    pha                                           // push it to the stack
    tya                                           // move y to a
    pha                                           // push it to the stack
    }                                             // 
                                                  // 
.macro popreg(){                                  // 
                                                  // 
    pla                                           // pull the y register from the stack
    tay                                           // move it to the y register
    pla                                           // pull the x register from the stack
    tax                                           // move it to the x register
    pla                                           // pull the acc from the stack
    plp                                           // pull the the processor status from the stack
    }                                             // 

*=$5400 "cuelist"


oscdata:  
list_item_1:            
        .text "/go 1                                   "
        .text "                                        "
        .byte 128,$41,0,0,0,0,0,0,0,0
list_item_2: 
        .text "/go 2                                   "
        .text "                                        "
        .byte 128,$42,0,0,0,0,0,0,0,0
list_item_3: 
        .text "/go 3                                   "
        .text "                                        "
        .byte 128,$43,0,0,0,0,0,0,0,0
list_item_4: 
        .text "/go 4                                   "
        .text "                                        "
        .byte 128,$44,0,0,0,0,0,0,0,0
list_item_5: 
        .text "/go 5                                   "
        .text "                                        "
        .byte 128,$45,0,0,0,0,0,0,0,0
list_item_6: 
        .text "/go 6                                   "
        .text "                                        "
        .byte 128,$46,0,0,0,0,0,0,0,0
list_item_7:        
        .text "/go 7                                   "
        .text "                                        "
        .byte 128,$47,0,0,0,0,0,0,0,0
list_item_8:       
        .text "/go 8                                   "
        .text "                                        "
        .byte 128,$48,0,0,0,0,0,0,0,0
list_item_9:       
        .text "/go 9                                   "
        .text "                                        "
        .byte 128,$49,0,0,0,0,0,0,0,0
list_item_10:       
        .text "/go 10                                  "
        .text "                                        "
        .byte 128,$4a,0,0,0,0,0,0,0,0
list_item_11:       
        .text "/go 11                                  "
        .text "                                        "
        .byte 128,$4b,0,0,0,0,0,0,0,0
list_item_12:       
        .text "/go 12                                  "
        .text "                                        "
        .byte 128,$4c,0,0,0,0,0,0,0,0
list_item_13:      
        .text "/go 13                                  "
        .text "                                        "
        .byte 128,$4d,0,0,0,0,0,0,0,0
list_item_14:       
        .text "/go 14                                  "
        .text "                                        "
        .byte 128,$4e,0,0,0,0,0,0,0,0
list_item_15:       
        .text "/go 15                                  "
        .text "                                        "
        .byte 128,$4f,0,0,0,0,0,0,0,0
list_item_16:       
        .text "/go 16                                  "
        .text "                                        "
        .byte 128,$50,0,0,0,0,0,0,0,0
list_item_17:      
        .text "/go 17                                  "
        .text "                                        "
        .byte 128,$51,0,0,0,0,0,0,0,0
list_item_18:       
        .text "/go 18                                  "
        .text "                                        "
        .byte 128,$52,0,0,0,0,0,0,0,0
list_item_19:       
        .text "/go 19                                  "
        .text "                                        "
        .byte 128,$53,0,0,0,0,0,0,0,0
list_item_20:       
        .text "/go 20                                  "
        .text "                                        "
        .byte 128,$54,0,0,0,0,0,0,0,0
list_item_21:      
        .text "/go 21                                  "
        .text "                                        "
        .byte 128,$55,0,0,0,0,0,0,0,0
list_item_22:       
        .text "/go 22                                  "
        .text "                                        "
        .byte 128,$56,0,0,0,0,0,0,0,0
list_item_23:       
        .text "/go 23                                  "
        .text "                                        "
        .byte 128,$57,0,0,0,0,0,0,0,0
list_item_24:       
        .text "/go 24                                  "
        .text "                                        "
        .byte 128,$58,0,0,0,0,0,0,0,0
list_item_25:      
        .text "/go 25                                  "
        .text "                                        "
        .byte 128,$59,0,0,0,0,0,0,0,0
list_item_26:       
        .text "/go 26                                  "
        .text "                                        "
        .byte 128,$5a,0,0,0,0,0,0,0,0


.align $100 


introscreen_chars:
.byte 32,111,111,111,111,111,111,111,111,111,111,111,111,111,78,77,32,32,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,111,32
.byte 32,101,32,32,32,32,32,32,32,32,32,32,32,32,32,32,77,78,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,106,32
.byte 32,101,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,106,32
.byte 32,101,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,106,32
.byte 32,101,32,32,32,32,32,32,233,160,160,160,206,160,223,32,233,160,160,160,160,160,223,32,233,160,160,206,160,160,223,32,32,32,32,32,32,32,106,32
.byte 32,101,32,32,32,32,32,233,160,223,67,67,233,215,105,233,160,223,67,67,233,160,105,233,160,223,67,67,233,160,105,32,32,32,32,32,32,32,106,32
.byte 32,101,32,32,32,32,233,160,206,105,32,233,209,105,233,160,206,105,32,32,32,32,233,160,206,105,32,32,67,67,32,32,32,32,32,32,32,32,106,32
.byte 32,101,32,32,32,233,160,206,105,32,233,206,105,233,160,160,160,160,160,160,105,233,160,206,105,32,32,32,32,32,233,160,175,105,233,105,233,105,106,32
.byte 32,101,32,32,233,192,192,105,32,233,192,105,32,67,67,67,67,233,192,105,233,192,209,105,32,32,32,32,32,233,105,67,67,233,105,233,105,32,106,32
.byte 32,101,32,233,206,206,105,32,233,160,105,233,160,105,32,32,233,206,105,233,160,215,105,32,32,233,160,105,233,175,160,105,233,215,173,105,32,32,106,32
.byte 32,101,233,160,160,195,195,195,209,105,233,160,160,160,206,160,160,105,233,160,206,160,160,160,160,160,105,233,105,233,105,32,67,233,105,32,32,32,106,32
.byte 32,101,95,160,160,160,160,160,105,32,95,160,160,215,160,160,105,32,95,209,195,195,160,160,160,105,233,160,160,105,32,32,233,105,32,32,32,32,106,32
.byte 32,101,32,67,67,67,67,67,32,32,32,67,67,67,67,67,32,32,32,67,67,67,67,67,67,32,67,67,67,32,32,32,67,32,32,32,32,32,106,32
.byte 32,101,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,106,32
.byte 32,101,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,112,110,32,32,32,32,32,32,112,110,32,32,32,32,112,110,32,106,32
.byte 32,101,32,112,67,114,67,114,67,114,67,110,112,67,114,67,114,114,114,67,114,125,66,112,67,114,67,114,67,115,109,114,67,114,67,115,66,32,106,32
.byte 32,101,112,66,66,66,66,66,66,66,66,66,66,109,115,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,112,115,112,115,66,66,66,32,106,32
.byte 32,101,66,66,66,66,66,66,112,115,66,66,107,110,66,66,66,66,66,66,66,66,66,66,107,115,66,66,66,66,66,66,66,66,66,66,66,32,106,32
.byte 32,101,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,66,32,106,32
.byte 32,101,66,109,67,115,112,113,67,113,113,125,109,67,113,67,113,67,113,113,113,67,125,109,67,113,67,113,113,113,67,113,125,109,67,113,125,32,106,32
.byte 32,101,109,64,64,109,125,64,64,32,6,15,18,32,25,15,21,18,32,3,15,13,13,15,4,15,18,5,32,54,52,32,64,64,64,125,32,32,106,32
.byte 32,101,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,106,32
.byte 32,101,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,106,32
.byte 32,101,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,32,78,77,32,32,32,32,32,32,32,32,32,8,17,14,106,32
.byte 32,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,119,77,78,32,32,119,119,119,119,119,119,119,119,119,119,119,119,119,32

introscreen_colors:
.byte 14,12,12,14,14,14,14,12,12,12,12,15,12,15,15,1,12,12,15,15,12,15,12,12,12,12,12,12,14,14,14,14,12,14,12,15,15,7,1,14
.byte 14,12,14,14,14,14,12,12,14,14,14,14,14,14,14,14,7,15,12,12,12,12,12,12,14,12,14,14,14,14,14,14,14,14,14,14,14,14,7,12
.byte 14,15,14,14,14,14,12,12,14,14,14,14,14,14,14,14,14,14,14,12,12,12,12,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,7,12
.byte 14,7,14,14,14,14,12,12,14,14,14,14,12,14,14,14,14,14,14,14,14,14,14,12,12,14,14,14,14,14,14,14,14,14,14,14,14,14,15,14
.byte 14,1,14,14,14,14,12,12,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,12,12,14,14,14,15,14
.byte 14,1,14,14,14,14,14,1,1,1,1,15,1,1,1,1,1,1,3,7,1,1,1,1,1,1,3,15,1,1,1,1,1,14,14,14,14,14,12,14
.byte 14,7,14,14,1,1,1,1,1,1,1,1,1,1,1,1,1,1,15,15,15,15,1,1,1,1,15,14,15,1,14,14,14,14,14,14,14,14,12,14
.byte 14,15,14,14,1,1,1,1,1,15,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,15,15,15,15,15,1,1,1,1,1,1,1,1,12,1
.byte 14,14,15,15,1,1,1,1,1,1,1,1,1,12,12,3,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1,7,1,1,1,1,1,1,12,7
.byte 14,14,14,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,1,1,1,1,1,1,1,1,7,1,12,1
.byte 1,14,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,15,7,3,3,15,15,14,14,14
.byte 1,14,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,15,15,3,3,15,14,14,14,14,14
.byte 1,12,7,11,7,3,14,11,11,11,11,11,11,14,7,3,11,11,11,11,3,7,7,12,11,11,14,3,7,14,14,14,14,14,14,14,14,14,14,14
.byte 7,12,14,14,14,14,14,14,14,14,14,14,14,14,14,15,15,15,14,14,14,7,15,15,15,15,15,7,7,7,15,15,14,14,14,7,15,11,12,14
.byte 7,12,14,14,14,14,14,14,14,14,14,14,14,14,14,15,15,15,14,14,14,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,12,14
.byte 1,12,15,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,15,14,14
.byte 1,12,14,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,14,14,5
.byte 1,12,11,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,5,14,5
.byte 1,14,11,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,5,12,5
.byte 1,14,11,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,15,15,5
.byte 1,14,11,14,14,3,3,14,11,14,15,15,15,15,15,15,15,15,15,1,1,1,1,1,1,1,1,1,1,1,1,15,11,14,14,11,11,5,12,5
.byte 4,12,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,12,4
.byte 4,12,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,12,4
.byte 1,12,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,14,7,1,12,12,14,14,14,14,14,14,15,11,11,11,15,14
.byte 14,1,7,7,15,15,12,12,12,12,12,12,12,12,12,12,12,12,12,12,14,14,14,15,12,12,1,1,7,15,14,14,12,12,15,15,15,7,7,14




VICEMODE:                     .byte 0
HAVEWIFI:                     .byte 0
TIMOUTERROR:                  .byte 0
MUTE:                         .byte 0
cuecount:                     .byte 0
editcount:                    .byte 0
cue_card_addr:                .word $6800
nxt_cue_card_addr:            .word $6800

cue_hsb_table:  .byte $54, $54, $54, $55, $55, $55, $56, $56, $56, $57, $57, $57, $58, $58, $58, $59, $59, $59, $5a, $5a, $5b, $5b, $5b, $5c, $5c, $5c, $54
cue_lsb_table:  .byte $00, $5a, $b4, $0e, $68, $c2, $1c, $76, $d0, $2a, $84, $de, $38, $92, $ec, $46, $a0, $fa, $54, $ae, $08, $62, $bc, $16, $70, $ca, $00



.segment Variables [start=$6650, virtual]
INCONFIG:                     .byte 0 
F7_EXIT_REQUEST:              .byte 0
READLIMIT:                    .byte 0
INVERT:                       .byte 0
CMD:                          .byte 0
HOME_LINE:                    .byte 0
HOME_COLM:                    .byte 0
LIMIT_LINE:                   .byte 0
LIMIT_COLM:                   .byte 0
CLEAR_FIELD_FLAG:             .byte 0
MENU_ID:                      .byte 0
CONFIG_STATUS:                .byte 0
DELAY:                        .byte 0
RXINDEX:                      .byte 0
RXFULL:                       .byte 0
TIMEOUTVALUE:                 .byte 0
CURSORCOLOR:                  .byte 0
SPLITBUFFER:                  .fill 40,32
RXBUFFER:                     .fill 256,128
TXBUFFER:                     .fill 256,128
TIMEOUT1:                     .byte 0
TIMEOUT2:                     .byte 0
