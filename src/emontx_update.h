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
#include <ESP8266AVRISP.h>

// Default port for AVR programming
#define EMONTX_AVRISP_PORT 328

// Reset pin for EmonTX (GPIO pin connected to EmonTX reset)
// Using GPIO 4 (D2 on NodeMCU) as default, can be configured
#ifndef EMONTX_RESET_PIN
#define EMONTX_RESET_PIN 4
#endif

// SPI frequency for programming (300kHz is safe default)
#define EMONTX_SPI_FREQ 300000

extern ESP8266AVRISP *emontx_programmer;

// Initialize the EmonTX firmware update system
void emontx_update_setup();

// Process any pending AVR programming requests
void emontx_update_loop();

// Get the current programming state
AVRISPState_t emontx_update_state();

// Check if programmer is available and ready
bool emontx_update_available();

#endif // _EMONTX_UPDATE_H
