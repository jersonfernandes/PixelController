/*
 * PixelInvaders tpm2.net implementation, Copyright (C) 2012 michael vogt <michu@neophob.com>
 * 
 * If you like this, make sure you check out http://www.pixelinvaders.ch
 *
 * ------------------------------------------------------------------------
 *
 * This is the SPI version, unlike software SPI which is configurable, hardware 
 * SPI works only on very specific pins. 
 *
 * On the Arduino Uno, Duemilanove, etc., clock = pin 13 and data = pin 11. 
 * For the Arduino Mega, clock = pin 52, data = pin 51. 
 * For the ATmega32u4 Breakout Board and Teensy, clock = pin B1, data = B2. 
 *
 * ------------------------------------------------------------------------
 *
 * This file is part of PixelController.
 *
 * PixelController is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * PixelController is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * 	
 *
 *
 */

#include <SPI.h>         
#include <Ethernet.h>
#include <EthernetUdp.h>  

//the lpd6803 library needs the timer1 library
//#include <TimerOne.h>
//#include <Neophob_LPD6803.h>

//get the lib here: https://github.com/neophob/WS2801-Library
#include <WS2801.h>

//#if UDP_TX_PACKET_MAX_SIZE < 64
//#error UDP packet size to small - modify UDP_TX_PACKET_MAX_SIZE in the file EthernetUdp.h and set buffers to 64 bytes 
//#endif

//define some tpm constants
#define TPM2NET_LISTENING_PORT 65506
#define TPM2NET_HEADER_SIZE 5
#define TPM2NET_HEADER_IDENT 0x9c
#define TPM2NET_CMD_DATAFRAME 0xda
#define TPM2NET_CMD_COMMAND 0xc0
#define TPM2NET_CMD_ANSWER 0xaa
#define TPM2NET_FOOTER_IDENT 0x36

#define NR_OF_PANELS 4
#define PIXELS_PER_PANEL 64

//as the arduino ethernet has only 2kb ram
//we must limit the maximal udp packet size
//a 64 pixel matrix needs 192 bytes data
#define UDP_PACKET_SIZE 512

// buffers for receiving and sending data
uint8_t packetBuffer[UDP_PACKET_SIZE]; //buffer to hold incoming packet,

//initialize pixels
//Neophob_LPD6803 strip = Neophob_LPD6803(PIXELS_PER_PANEL*NR_OF_PANELS);
WS2801 strip = WS2801(PIXELS_PER_PANEL*NR_OF_PANELS);

//network stuff, TODO: add dhcp/bonjour support
byte mac[] = { 0xBE, 0x00, 0xBE, 0x00, 0xBE, 0x01 };
IPAddress ip(192, 168, 111, 177);
EthernetUDP Udp;



void setup() {  
  Serial.begin(115200);
  Serial.println("Hello!");

  // start the Ethernet and UDP:
  Ethernet.begin(mac,ip);
  Udp.begin(TPM2NET_LISTENING_PORT);
  memset(packetBuffer, 0, UDP_PACKET_SIZE);

  strip.begin();
  showInitImage();      // display some colors
  
  Serial.println("Setup done");
}


void loop() {
  // if there's data available, read a packet
  int packetSize = Udp.parsePacket();
  
  //tpm2 header size is 5 bytes
  if (packetSize>TPM2NET_HEADER_SIZE) {
    //TODO get packet nummer, use only if currentNr>oldNr
    
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    
    // read the packet into packetBufffer
    Udp.read(packetBuffer, UDP_PACKET_SIZE);
    
    // -- Header check
    
    //check header byte
    if (packetBuffer[0]!=TPM2NET_HEADER_IDENT) {
      Serial.print("Invalid header ident ");
      Serial.println(packetBuffer[0], HEX);
      return;
    }
    
    //check command
    if (packetBuffer[1]!=TPM2NET_CMD_DATAFRAME) {
      Serial.print("Invalid block type ");
      Serial.println(packetBuffer[1], HEX);
      return;
    }
    
    uint16_t frameSize = ((packetBuffer[3] << 0) & 0xFF) + ((packetBuffer[2] << 8) & 0xFF00);
    Serial.print("Framesize ");
    Serial.println(frameSize, HEX);

    //I guess packetNumber is violate the spec..
    uint8_t packetNumber = packetBuffer[4];
    Serial.print("packetNumber ");
    Serial.println(packetNumber, HEX);

    uint16_t currentLed = packetNumber*PIXELS_PER_PANEL;
    int x=TPM2NET_HEADER_SIZE;
    for (byte i=0; i < frameSize; i++) {
      strip.setPixelColor(currentLed++, packetBuffer[x], packetBuffer[x+1], packetBuffer[x+2]);
      x+=3;
    }
    strip.show();   // write all the pixels out

    //check footer
    frameSize += TPM2NET_HEADER_SIZE;
    if (packetBuffer[frameSize]!=TPM2NET_FOOTER_IDENT) {
      Serial.print("Invalid footer ident ");
      Serial.println(packetBuffer[frameSize], HEX);
      return;
    }
  }
}

// --------------------------------------------
//     create initial image
// --------------------------------------------
void showInitImage() {
  //just create some boring colors
  for (int i=0; i < strip.numPixels(); i++) {
    strip.setPixelColor(i, (i)%255, (i*2)%255, (i*4)%255);
  }    
  // Update the strip
  strip.show();
}


