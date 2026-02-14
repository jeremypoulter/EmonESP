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
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>

ESP8266AVRISP *emontx_programmer = NULL;

void emontx_update_setup() {
  DBUGLN("Initializing EmonTX firmware update system");
  
  // Create the programmer instance
  emontx_programmer = new ESP8266AVRISP(
    EMONTX_AVRISP_PORT,
    EMONTX_RESET_PIN,
    EMONTX_SPI_FREQ,
    false,  // reset_state (false = target runs normally)
    false   // reset_activehigh (false = active low reset)
  );
  
  // Let the AVR run (don't hold it in reset)
  emontx_programmer->setReset(false);
  
  // Start listening for programming connections
  emontx_programmer->begin();
  
  // Register mDNS service for discovery
  MDNS.addService("avrisp", "tcp", EMONTX_AVRISP_PORT);
  
  DBUGF("EmonTX programmer ready on port %d", EMONTX_AVRISP_PORT);
  DBUGF("Use: avrdude -c arduino -p m328p -P net:%s:%d -U flash:w:firmware.hex:i", 
        WiFi.localIP().toString().c_str(), EMONTX_AVRISP_PORT);
}

void emontx_update_loop() {
  if (emontx_programmer == NULL) {
    return;
  }
  
  static AVRISPState_t last_state = AVRISP_STATE_IDLE;
  AVRISPState_t new_state = emontx_programmer->update();
  
  if (last_state != new_state) {
    switch (new_state) {
      case AVRISP_STATE_IDLE:
        DBUGLN("[EmonTX] Programmer idle");
        break;
      case AVRISP_STATE_PENDING:
        DBUGLN("[EmonTX] Programming connection pending");
        break;
      case AVRISP_STATE_ACTIVE:
        DBUGLN("[EmonTX] Programming in progress");
        break;
    }
    last_state = new_state;
  }
  
  // Serve programming requests when not idle
  if (last_state != AVRISP_STATE_IDLE) {
    emontx_programmer->serve();
  }
}

AVRISPState_t emontx_update_state() {
  if (emontx_programmer == NULL) {
    return AVRISP_STATE_IDLE;
  }
  return emontx_programmer->update();
}

bool emontx_update_available() {
  return (emontx_programmer != NULL);
}
