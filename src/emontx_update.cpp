/*
 * -------------------------------------------------------------------
 * EmonESP Serial to Emoncms gateway  
 * -------------------------------------------------------------------
 * Adaptation of Chris Howells OpenEVSE ESP Wifi
 * by Trystan Lea, Glyn Hudson, OpenEnergyMonitor
 * All adaptation GNU General Public License as below.
 *
 * -------------------------------------------------------------------
 *
 * This file is part of OpenEnergyMonitor.org project.
 * EmonESP is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * EmonESP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with EmonESP; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "emontx_update.h"
#include "emonesp.h"
#include "debug.h"
#include <FS.h>

// STK500 protocol constants
// Based on: https://github.com/Optiboot/optiboot/blob/master/optiboot/bootloaders/optiboot/stk500.h
#define STK_OK              0x10
#define STK_INSYNC          0x14
#define STK_CRC_EOP         0x20
#define STK_GET_SYNC        0x30
#define STK_ENTER_PROGMODE  0x50
#define STK_LEAVE_PROGMODE  0x51
#define STK_LOAD_ADDRESS    0x55
#define STK_PROG_PAGE       0x64
#define STK_READ_SIGN       0x75

#define BOOTLOADER_TIMEOUT  1000  // ms

static uint8_t resetPin = EMONTX_RESET_PIN;
static uint32_t normalBaudRate = 115200;

// Helper functions
static void resetTarget() {
  digitalWrite(resetPin, LOW);
  delay(50);
  digitalWrite(resetPin, HIGH);
  delay(50);
}

static bool waitForResponse(uint8_t expected, uint32_t timeout_ms) {
  uint32_t start = millis();
  while (millis() - start < timeout_ms) {
    if (EMONTX_PORT.available()) {
      uint8_t c = EMONTX_PORT.read();
      if (c == expected) {
        return true;
      }
    }
    yield();
  }
  return false;
}

static bool sendCommand(uint8_t cmd) {
  EMONTX_PORT.write(cmd);
  EMONTX_PORT.write(STK_CRC_EOP);
  EMONTX_PORT.flush();
  
  if (!waitForResponse(STK_INSYNC, BOOTLOADER_TIMEOUT)) {
    return false;
  }
  if (!waitForResponse(STK_OK, BOOTLOADER_TIMEOUT)) {
    return false;
  }
  return true;
}

static bool syncBootloader() {
  // Try multiple times to sync with bootloader
  for (int attempt = 0; attempt < 5; attempt++) {
    // Clear any pending data
    while (EMONTX_PORT.available()) {
      EMONTX_PORT.read();
    }
    
    if (sendCommand(STK_GET_SYNC)) {
      return true;
    }
    delay(50);
  }
  return false;
}

static bool setAddress(uint16_t address) {
  // Address in words (not bytes) for ATmega
  uint16_t wordAddress = address >> 1;
  
  EMONTX_PORT.write(STK_LOAD_ADDRESS);
  EMONTX_PORT.write(wordAddress & 0xFF);
  EMONTX_PORT.write((wordAddress >> 8) & 0xFF);
  EMONTX_PORT.write(STK_CRC_EOP);
  EMONTX_PORT.flush();
  
  if (!waitForResponse(STK_INSYNC, BOOTLOADER_TIMEOUT)) {
    return false;
  }
  if (!waitForResponse(STK_OK, BOOTLOADER_TIMEOUT)) {
    return false;
  }
  return true;
}

static bool programPage(uint16_t address, uint8_t* data, uint16_t length) {
  if (!setAddress(address)) {
    return false;
  }
  
  EMONTX_PORT.write(STK_PROG_PAGE);
  EMONTX_PORT.write((length >> 8) & 0xFF);
  EMONTX_PORT.write(length & 0xFF);
  EMONTX_PORT.write('F');  // Flash memory
  
  for (uint16_t i = 0; i < length; i++) {
    EMONTX_PORT.write(data[i]);
  }
  
  EMONTX_PORT.write(STK_CRC_EOP);
  EMONTX_PORT.flush();
  
  if (!waitForResponse(STK_INSYNC, BOOTLOADER_TIMEOUT)) {
    return false;
  }
  if (!waitForResponse(STK_OK, BOOTLOADER_TIMEOUT)) {
    return false;
  }
  return true;
}

// Parse Intel HEX format and program the device
// Simple parser for :LLAAAATT[DD...]CC format
static int programHexFile(File& hexFile) {
  uint8_t pageBuffer[128];  // ATmega328 has 128-byte pages
  uint16_t pageAddress = 0;
  uint16_t pageIndex = 0;
  bool pageStarted = false;
  
  while (hexFile.available()) {
    String line = hexFile.readStringUntil('\n');
    line.trim();
    
    if (line.length() == 0 || line[0] != ':') {
      continue;
    }
    
    // Parse HEX record
    uint8_t byteCount = strtol(line.substring(1, 3).c_str(), NULL, 16);
    uint16_t address = strtol(line.substring(3, 7).c_str(), NULL, 16);
    uint8_t recordType = strtol(line.substring(7, 9).c_str(), NULL, 16);
    
    if (recordType == 0x00) {  // Data record
      // Check if this is a new page
      if (!pageStarted || (address & 0xFF80) != (pageAddress & 0xFF80)) {
        // Write previous page if it exists
        if (pageStarted && pageIndex > 0) {
          if (!programPage(pageAddress, pageBuffer, pageIndex)) {
            return FLASH_ERROR_PAGE_WRITE;
          }
        }
        
        // Start new page
        pageAddress = address;
        pageIndex = 0;
        pageStarted = true;
        memset(pageBuffer, 0xFF, sizeof(pageBuffer));
      }
      
      // Add data to page buffer
      for (uint8_t i = 0; i < byteCount; i++) {
        uint8_t dataByte = strtol(line.substring(9 + i*2, 11 + i*2).c_str(), NULL, 16);
        uint16_t offset = (address + i) & 0x7F;  // Offset within page
        pageBuffer[offset] = dataByte;
        if (offset >= pageIndex) {
          pageIndex = offset + 1;
        }
      }
    } else if (recordType == 0x01) {  // End of file
      // Write final page
      if (pageStarted && pageIndex > 0) {
        // Pad to page boundary
        while (pageIndex < 128) {
          pageBuffer[pageIndex++] = 0xFF;
        }
        if (!programPage(pageAddress, pageBuffer, 128)) {
          return FLASH_ERROR_PAGE_WRITE;
        }
      }
      break;
    }
    
    yield();  // Allow ESP8266 to handle WiFi, etc.
  }
  
  return FLASH_SUCCESS;
}

void emontx_update_setup() {
  pinMode(resetPin, OUTPUT);
  digitalWrite(resetPin, HIGH);
  
  DBUGLN("EmonTX serial programmer initialized");
  DBUGF("Reset pin: GPIO%d", resetPin);
}

int emontx_flash_firmware(const char* hexFilePath) {
  DBUGF("Starting EmonTX firmware update from: %s", hexFilePath);
  
  // Check if file exists
  if (!SPIFFS.exists(hexFilePath)) {
    DBUGLN("Firmware file not found");
    return FLASH_ERROR_FILE_NOT_FOUND;
  }
  
  File hexFile = SPIFFS.open(hexFilePath, "r");
  if (!hexFile) {
    DBUGLN("Failed to open firmware file");
    return FLASH_ERROR_FILE_READ;
  }
  
  // Store current baud rate
  normalBaudRate = EMONTX_PORT.baudRate();
  
  // Switch to programming baud rate
  EMONTX_PORT.flush();
  EMONTX_PORT.end();
  EMONTX_PORT.begin(EMONTX_PROG_BAUD_RATE);
  
  // Reset target to enter bootloader
  resetTarget();
  
  // Clear serial buffer
  while (EMONTX_PORT.available()) {
    EMONTX_PORT.read();
  }
  
  // Sync with bootloader
  DBUGLN("Syncing with bootloader...");
  if (!syncBootloader()) {
    DBUGLN("Failed to sync with bootloader");
    hexFile.close();
    EMONTX_PORT.end();
    EMONTX_PORT.begin(normalBaudRate);
    return FLASH_ERROR_SYNC;
  }
  
  DBUGLN("Bootloader synchronized");
  
  // Enter programming mode
  sendCommand(STK_ENTER_PROGMODE);
  
  // Program the firmware
  DBUGLN("Programming firmware...");
  int result = programHexFile(hexFile);
  
  hexFile.close();
  
  if (result != FLASH_SUCCESS) {
    DBUGF("Programming failed with error: %d", result);
    EMONTX_PORT.end();
    EMONTX_PORT.begin(normalBaudRate);
    return result;
  }
  
  // Leave programming mode
  DBUGLN("Leaving programming mode...");
  if (!sendCommand(STK_LEAVE_PROGMODE)) {
    DBUGLN("Warning: Failed to leave programming mode cleanly");
    // Don't return error - firmware is already written
  }
  
  // Restore normal baud rate
  EMONTX_PORT.end();
  EMONTX_PORT.begin(normalBaudRate);
  
  // Reset to start new firmware
  resetTarget();
  
  DBUGLN("Firmware update completed successfully");
  return FLASH_SUCCESS;
}

const char* emontx_flash_error_string(int errorCode) {
  switch (errorCode) {
    case FLASH_SUCCESS:
      return "Success";
    case FLASH_ERROR_FILE_NOT_FOUND:
      return "Firmware file not found";
    case FLASH_ERROR_FILE_READ:
      return "Failed to read firmware file";
    case FLASH_ERROR_SYNC:
      return "Failed to sync with bootloader";
    case FLASH_ERROR_ADDRESS:
      return "Failed to set flash address";
    case FLASH_ERROR_PAGE_WRITE:
      return "Failed to write flash page";
    case FLASH_ERROR_LEAVE_PROG:
      return "Failed to leave programming mode";
    default:
      return "Unknown error";
  }
}
