/*******************************************************************************
   ModbusTCP.h

   Lightweight Modbus TCP 32-bit slave for Arduino.

   Three MODBUS function codes are supported; reading (0x04) and writing (0x10)
   of holding registers and function 0x06 for compatiblilty.

   Three Modbus encodings are supported; UInt16, UInt32 and Float (Float and
   Double are identical on Arduino Uno). With these three primitive types you
   can also support Int16, Int32, or packed bits (coils).

   Holding register addressing begins at 40001 and are contiguous.

   Data is stored in native MODBUS UInt16 registers. To make it less error
   prone, get and set functions are included for each encoding that also do
   error checking. If you're crushed for resources feel free to comment out
   the define statements and write to the registers directly.

   Testing
   This code was developed and tested using modpoll and Inductive Automation
   Ignition quick client running in trial mode.

   http://www.modbusdriver.com/modpoll.html
   https://www.inductiveautomation.com/scada-software

   Credits

   https://code.google.com/p/mudbus/

   Thanks to Dee Wykoff and Martin Pettersson for making the Mudbus library
   available as a place to start.

   MIT License
   Copyright (c) 2014, 2017 Ductsoup
   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/

//
// Application specific, adjust as necessary
//
#define MB_PORT 502
#define MB_REGISTERS_MAX 32
uint16_t mb_reg[MB_REGISTERS_MAX];

//
// Comment out features you don't need to save space
//
#define MB_DEBUG
#define MB_Float
#define MB_UInt32
#define MB_UInt16

//
// Adafruit Feather M0 WiFi - ATSAMD21 + ATWINC1500
// https://www.adafruit.com/product/3010
//
#include <SPI.h>
#include <WiFi101.h>
#define WINC_CS   8
#define WINC_IRQ  7
#define WINC_RST  4
#define WINC_EN   2


/*******************************************************************************
  It's probably best not to edit below here unless you're sure you know what
  you're doing
 *******************************************************************************/

//
// WiFi server
//
int status = WL_IDLE_STATUS;
WiFiServer server(MB_PORT);
boolean alreadyConnected = false; // whether or not the client was connected previously

/*******************************************************************************

   MODBUS Reference

   http://www.csimn.com/CSI_pages/Modbus101.html

   Modicon convention notation

   0x = Coil = 00001-09999
   1x = Discrete Input = 10001-19999
   3x = Input Register = 30001-39999
   4x = Holding Register = 40001-49999

   Modbus protocol defines a holding register as 16 bits wide; however, there is
   a widely used defacto standard for reading and writing data wider than 16
   bits. The most common are IEEE 754 floating point, and 32-bit integer. The
   convention may also be extended to double precision floating point and 64-bit
   integer data.

   http://www.freemodbus.org/

   ----------------------- MBAP Header --------------------------------------

   <------------------------ MODBUS TCP/IP ADU(1) ------------------------->
   <----------- MODBUS PDU (1') ---------------->
   +-----------+---------------+------------------------------------------+
   | TID | PID | Length | UID  |Code | Data                               |
   +-----------+---------------+------------------------------------------+
   |     |     |        |      |
   (2)   (3)   (4)      (5)    (6)
   (2)  ... MB_TCP_TID          = 0 (Transaction Identifier - 2 Byte)
   (3)  ... MB_TCP_PID          = 2 (Protocol Identifier - 2 Byte)
   (4)  ... MB_TCP_LEN          = 4 (Number of bytes - 2 Byte)
   (5)  ... MB_TCP_UID          = 6 (Unit Identifier - 1 Byte)
   (6)  ... MB_TCP_FUNC         = 7 (Modbus Function Code)

   (1)  ... Modbus TCP/IP Application Data Unit
   (1') ... Modbus Protocol Data Unit

 *******************************************************************************/

//
// MODBUS request buffer
//
uint8_t mb_adu[260];
int16_t iTXLen = 0;       // response packet length

//
// MODBUS Function Codes
//
#define MB_FC_NONE 0
#define MB_FC_READ_COILS 1
#define MB_FC_READ_DISCRETE_INPUT 2
#define MB_FC_READ_REGISTERS 3 // implemented
#define MB_FC_READ_INPUT_REGISTERS 4
#define MB_FC_WRITE_COIL 5
#define MB_FC_WRITE_REGISTER 6 // implemented
#define MB_FC_WRITE_MULTIPLE_COILS 15
#define MB_FC_WRITE_MULTIPLE_REGISTERS 16 // implemented
//
// MODBUS Error Codes
//
#define MB_EC_NONE 0
#define MB_EC_ILLEGAL_FUNCTION 1
#define MB_EC_ILLEGAL_DATA_ADDRESS 2
#define MB_EC_ILLEGAL_DATA_VALUE 3
#define MB_EC_SLAVE_DEVICE_FAILURE 4
//
// MODBUS MBAP offsets
//
#define MB_TCP_TID          0 // ignored
#define MB_TCP_PID          2 // ignored
#define MB_TCP_LEN          4
#define MB_TCP_UID          6 // ignored
#define MB_TCP_FUNC         7
#define MB_TCP_DATA         8

#ifdef MB_Float
int setFloat(uint16_t iAddress, float fValue) {

  int iRet;
  union {
    float f;
    uint16_t w[2];
  }
  fw;

  for (iRet = 0 ; !iRet ; iRet = 2) {
    if ((iAddress -= 40001) < 0 || (iAddress + 1) >= MB_REGISTERS_MAX) // invalid address
      break;
    fw.f = fValue;
    mb_reg[iAddress++] = fw.w[0];
    mb_reg[iAddress] = fw.w[1];
  }
  return (iRet);
}
float getFloat(uint16_t iAddress) {

  union {
    float fRet;
    uint16_t w[2];
  }
  fw;

  if ((iAddress -= 40001) < 0 || (iAddress + 1) >= MB_REGISTERS_MAX) // invalid address
    fw.fRet = NULL;
  else {
    fw.w[0] = mb_reg[iAddress++];
    fw.w[1] = mb_reg[iAddress];
  }
  return (fw.fRet);
}
#endif
#ifdef MB_UInt32
int setU32(uint16_t iAddress, uint32_t iValue) {

  int iRet;
  for (iRet = 0 ; !iRet ; iRet = 2) {
    if ((iAddress -= 40001) < 0 || (iAddress + 1) >= MB_REGISTERS_MAX) // invalid address
      break;
    mb_reg[iAddress++] = iValue & 0xffff;
    mb_reg[iAddress] = iValue >> 16;
  }
  return (iRet);
}
uint32_t getU32(uint16_t iAddress) {

  uint32_t lRet;

  if ((iAddress -= 40001) < 0 || (iAddress + 1) >= MB_REGISTERS_MAX) // invalid address
    lRet = NULL;
  else {
    lRet = (long(mb_reg[iAddress++]) | (long(mb_reg[iAddress + 1]) << 16));
  }
  return (lRet);
}
#endif
#ifdef MB_UInt16
int setU16(uint16_t iAddress, uint16_t iValue) {

  int iRet;
  for (iRet = 0 ; !iRet ; iRet = 1) {
    if ((iAddress -= 40001) < 0 || (iAddress >= MB_REGISTERS_MAX)) // invalid address
      break;
    mb_reg[iAddress] = iValue;
  }
  return (iRet);
}
uint16_t getU16(uint16_t iAddress) {

  uint16_t iRet;

  if ((iAddress -= 40001) < 0 || (iAddress >= MB_REGISTERS_MAX)) // invalid address
    iRet = NULL;
  else
    iRet = mb_reg[iAddress];

  return (iRet);
}
#endif
#ifdef MB_DEBUG
void printHex(int num, int precision) {

  char tmp[16];
  char format[128];

  sprintf(format, "0x%%0%dX ", precision);
  sprintf(tmp, format, num);
  Serial.print(tmp);
}
void printMB(char *s, int n) {

  int i;

  Serial.print(s);
  for (i = 0 ; i < n ; i++) {
    printHex(mb_adu[i], 2);
    if (i == 6) Serial.print("- ");
    if (i == 7) Serial.print("- ");
  }
  Serial.println("");
}
#endif

//
//  Process requests and prepare a response
//
void modbus_proc(void) {

  int16_t i, iStart, iQty;
  uint8_t *ptr, iFC = MB_FC_NONE, iEC = MB_EC_NONE;

  iFC = mb_adu[MB_TCP_FUNC];
  //
  // Handle request
  //
  switch (iFC) {
    case MB_FC_NONE:
      break;
    case MB_FC_READ_REGISTERS:
      //
      // 03 (0x03) Read Holding Registers
      //
      // modpoll -m tcp -t 4:float -r 40001 -c 1 -1 192.168.x.x
      //
      //     [TransID] [ProtID-] [Length-] [Un] [FC] [Start--] [Qty----]
      // RX: 0x00 0x01 0x00 0x00 0x00 0x06 0x01 0x03 0x9C 0x40 0x00 0x02
      //
      //     [TransID] [ProtID-] [Length-] [Un] [FC] [Bc] [float------------]
      // TX: 0x00 0x01 0x00 0x00 0x00 0x07 0x01 0x03 0x04 0x20 0x00 0x47 0xF1
      //
      // 123456.0 = 0x00 0x20 0xF1 0x47 (IEEE 754)
      //
      // Unpack the start and length
      //
      ptr = mb_adu + MB_TCP_DATA;
      iStart = word(*ptr++, *ptr++) - 40000;
      iQty = 2 * word(*ptr++, *ptr);
      //
      // check for valid register addresses
      //
      if (iStart < 0 || (iStart + iQty / 2 - 1) > MB_REGISTERS_MAX) {
        iEC = MB_EC_ILLEGAL_DATA_ADDRESS;
        break;
      }
      //
      // Write data length
      //
      ptr = mb_adu + MB_TCP_DATA;
      *ptr++ = iQty;
      //
      // Write data
      //
      for (i = 0 ; i < iQty / 2 ; i++) {
        *ptr++ = highByte(mb_reg[iStart + i]);
        *ptr++ =  lowByte(mb_reg[iStart + i]);
      }
      iTXLen = iQty + 9;
      break;

    case MB_FC_WRITE_REGISTER:
      //
      // 06 (0x06) Write register
      //
      ptr = mb_adu + MB_TCP_DATA;
      iStart = word(*ptr++, *ptr++) - 40000;
      //
      // check for valid register addresses
      //
      if (iStart < 0 || (iStart - 1) > MB_REGISTERS_MAX) {
        iEC = MB_EC_ILLEGAL_DATA_ADDRESS;
        break;
      }
      // Unpack and store data
      //
      mb_reg[iStart] = word(*ptr++, *ptr);
      //
      // Build a response
      //
      iTXLen = 12;
      break;

    case MB_FC_WRITE_MULTIPLE_REGISTERS:
      //
      // 16 (0x10) Write Multiple registers
      //
      // modpoll -m tcp -t 4:float -r 40001 -c 1 -1 192.168.x.x 123456.0
      //
      //     [TransID] [ProtID-] [Length-] [Un] [FC] [Start--] [Qty----] [Bc] [float------------]
      // RX: 0x00 0x01 0x00 0x00 0x00 0x0B 0x01 0x10 0x9C 0x40 0x00 0x02 0x04 0x20 0x00 0x47 0xF1
      //
      // 123456.0 = 0x00 0x20 0xF1 0x47 (IEEE 754)
      //
      // Unpack the start and length
      //
      ptr = mb_adu + MB_TCP_DATA;
      iStart = word(*ptr++, *ptr++) - 40000;
      iQty = 2 * word(*ptr++, *ptr);
      //
      // check for valid register addresses
      //
      if (iStart < 0 || (iStart + iQty / 2 - 1) > MB_REGISTERS_MAX) {
        iEC = MB_EC_ILLEGAL_DATA_ADDRESS;
        break;
      }
      //
      // Unpack and store data
      //
      ptr = mb_adu + MB_TCP_DATA + 5;
      // todo: check for valid length
      for (i = 0 ; i < iQty / 2 ; i++) {
        mb_reg[iStart + i] = word(*ptr++, *ptr++);
      }
      //
      // Build a response
      //
      iTXLen = 12;
      break;

    default:
      iEC = MB_EC_ILLEGAL_FUNCTION;
      break;
  }
  //
  // Build exception response if necessary because we were too
  // lazy to do it earlier. Other responses should already be
  // built.
  //
  if (iEC) {
    ptr = mb_adu + MB_TCP_FUNC;
    *ptr = *ptr++ | 0x80;        // flag the function code
    *ptr = iEC;                  // write the exception code
    iTXLen = 9;
  }
  //
  // If there's a response, transmit it
  //
  if (iFC) {
    ptr = mb_adu + MB_TCP_LEN;    // write the header length
    *ptr++ = 0x00;
    *ptr = iTXLen - MB_TCP_UID;
#ifdef MB_DEBUG
    printMB("TX: ", word(mb_adu[MB_TCP_LEN], mb_adu[MB_TCP_LEN + 1]) + MB_TCP_UID);
    Serial.println();
#endif
  }
}

//
//  Check for and handle any requests
//
void modbus_run(void) {
  int16_t i = 0;
  WiFiClient client = server.available();
  if (client) {
    if (!alreadyConnected) {
      client.flush();
      Serial.println("new client");
      alreadyConnected = true;
    }
    if (client.available() > 0) {
      //i = 0;
      while (client.available())
        mb_adu[i++] = client.read();
#ifdef MB_DEBUG
      printMB("RX: ", word(mb_adu[MB_TCP_LEN], mb_adu[MB_TCP_LEN + 1]) + 6);
#endif
      modbus_proc();
      client.write(mb_adu, iTXLen);
    }
  }
}

//
//  Get the WiFi going
//
void modbus_begin(void) {
  int i = -1;
  while (true) {
    WiFi.setPins(WINC_CS, WINC_IRQ, WINC_RST, WINC_EN);
#ifdef MB_DEBUG
    Serial.print("WiFi Status: ");
    Serial.println(status);
#endif
    if (WiFi.status() == WL_NO_SHIELD) {
#ifdef MB_DEBUG
      Serial.println("WiFi shield not present");
#endif
      break;
    }
    while ( status != WL_CONNECTED) {
      ++i = i >= sizeof(access_points) / (2 * sizeof(char*)) ? 0 : i;
#ifdef MB_DEBUG
      Serial.print("Attempting to connect to SSID: ");
      Serial.println(access_points[i][0]);
#endif
      status = WiFi.begin(access_points[i][0], access_points[i][1]);
      delay(10000);
    }
    break;
  }
#ifdef MB_DEBUG
  Serial.print("WiFi Status: ");
  Serial.println(status);
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);
  IPAddress ip = WiFi.localIP();
  Serial.print("IP:  ");
  Serial.println(ip);
#endif
  server.begin();
}

