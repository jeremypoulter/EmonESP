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

#ifndef _EMONTX_UPDATE_H
#define _EMONTX_UPDATE_H

#include <Arduino.h>

// Reset pin for EmonTX (GPIO pin connected to EmonTX reset)
// Using GPIO 4 (D2 on NodeMCU) as default, can be configured
#ifndef EMONTX_RESET_PIN
#define EMONTX_RESET_PIN 4
#endif

// Baud rate for programming (38400 works well for 8MHz ATmega328)
#define EMONTX_PROG_BAUD_RATE 38400

// Flash result codes
#define FLASH_SUCCESS 0
#define FLASH_ERROR_FILE_NOT_FOUND 1
#define FLASH_ERROR_FILE_READ 2
#define FLASH_ERROR_SYNC 3
#define FLASH_ERROR_ADDRESS 4
#define FLASH_ERROR_PAGE_WRITE 5
#define FLASH_ERROR_LEAVE_PROG 6

// Initialize the EmonTX firmware update system
void emontx_update_setup();

// Flash firmware from a hex file in the filesystem
// Returns FLASH_SUCCESS (0) on success, error code otherwise
int emontx_flash_firmware(const char* hexFilePath);

// Get human-readable error message for flash result code
const char* emontx_flash_error_string(int errorCode);

#endif // _EMONTX_UPDATE_H
