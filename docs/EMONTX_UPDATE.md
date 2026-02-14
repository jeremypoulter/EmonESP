# EmonTX Firmware Update via Serial

## Overview

EmonESP now supports updating the firmware on connected EmonTX devices (ATmega328-based) via the existing UART/serial connection. This uses the STK500 protocol to communicate with the Optiboot bootloader on the EmonTX.

## How It Works

The EmonESP acts as a serial programmer, implementing the STK500 protocol to communicate with the Optiboot bootloader on the EmonTX. The firmware is uploaded as an Intel HEX file to the ESP8266's filesystem, then programmed to the EmonTX over the serial connection.

### Key Features

- **Serial Programming**: Uses the existing UART connection between EmonESP and EmonTX
- **STK500 Protocol**: Compatible with standard Arduino bootloaders (Optiboot)
- **Web Upload**: Upload firmware hex files via HTTP
- **Intel HEX Format**: Supports standard Arduino compiler output format

## Hardware Requirements

### Connections

The EmonTX must be connected to the EmonESP via UART:
- EmonTX TX → ESP8266 RX (Serial)
- EmonTX RX → ESP8266 TX (Serial)
- EmonTX RESET → ESP8266 GPIO4 (configurable)
- GND common

**Important:** The EmonTX must have the Optiboot bootloader installed (standard on Arduino boards).

### Voltage Levels

If the EmonTX runs at 5V and ESP8266 at 3.3V, you need a voltage divider or level shifter on the EmonTX TX → ESP8266 RX line to protect the ESP8266.

Example voltage divider:
- EmonTX TX → 1kΩ resistor → ESP8266 RX
- ESP8266 RX → 2kΩ resistor → GND

## Configuration

The reset pin can be configured by defining `EMONTX_RESET_PIN` in your build flags:

```ini
build_flags = 
  -DEMONTX_RESET_PIN=5  ; Use GPIO5 instead of default GPIO4
```

The programming baud rate defaults to 38400, which works well for 8MHz ATmega328 chips with Optiboot.

## Usage

### Step 1: Compile Your EmonTX Firmware

Compile your EmonTX firmware using the Arduino IDE or PlatformIO. Make sure to export the compiled binary in Intel HEX format (`.hex` file).

In Arduino IDE: Sketch → Export Compiled Binary

### Step 2: Upload Firmware to EmonESP

Upload the hex file to the EmonESP via HTTP POST:

```bash
curl -F "file=@firmware.hex" http://<ESP_IP>/emontx/upload
```

Or use a web form/interface.

### Step 3: Flash the Firmware

Trigger the programming process:

```bash
curl -X POST http://<ESP_IP>/emontx/flash
```

Response example:
```json
{
  "success": true,
  "code": 0,
  "message": "Success"
}
```

### Complete Example

```bash
# Upload firmware
curl -F "file=@emontx_v3.hex" http://192.168.1.100/emontx/upload

# Flash to EmonTX
curl -X POST http://192.168.1.100/emontx/flash
```

## Programming Process

When you trigger `/emontx/flash`, the EmonESP:

1. Switches the serial port to programming baud rate (38400)
2. Resets the EmonTX to enter the bootloader
3. Synchronizes with the Optiboot bootloader using STK500
4. Parses the Intel HEX file
5. Programs each page of flash memory
6. Leaves programming mode
7. Restores normal serial communication
8. Resets the EmonTX to start the new firmware

The entire process typically takes 10-30 seconds depending on firmware size.

## Troubleshooting

### "Failed to sync with bootloader"

**Possible causes:**
- EmonTX doesn't have Optiboot bootloader installed
- Wrong baud rate for your EmonTX clock speed
- Reset pin not properly connected
- Serial connection issues

**Solutions:**
- Verify Optiboot is installed on EmonTX
- Try different baud rates (19200, 38400, 57600, 115200)
- Check reset pin wiring
- Verify TX/RX connections

### "Failed to write flash page"

**Possible causes:**
- Corrupted hex file
- Serial communication errors
- Timing issues

**Solutions:**
- Re-export the hex file from Arduino IDE
- Check serial connection quality
- Ensure proper voltage levels (use level shifters if needed)

### Programming Never Completes

**Possible causes:**
- EmonTX is not being reset properly
- Bootloader timeout (bootloader only waits ~1 second after reset)

**Solutions:**
- Check reset pin connection
- Ensure reset pin is HIGH normally, pulled LOW to reset
- Verify GPIO pin number matches `EMONTX_RESET_PIN`

## Technical Details

### STK500 Protocol

The implementation uses a subset of the STK500v1 protocol that Optiboot supports:
- `STK_GET_SYNC`: Synchronize with bootloader
- `STK_ENTER_PROGMODE`: Enter programming mode
- `STK_LOAD_ADDRESS`: Set flash address
- `STK_PROG_PAGE`: Program a page of flash
- `STK_LEAVE_PROGMODE`: Leave programming mode

### Intel HEX Format

The parser supports standard Intel HEX format:
- `:LLAAAATT[DD...]CC` format
- Record types: Data (00), End of File (01)
- 128-byte page size (ATmega328 flash page size)

### Baud Rates

Recommended baud rates based on ATmega328 clock speed:
- 8 MHz (internal): 38400
- 16 MHz (external crystal): 57600 or 115200

## API Reference

### POST /emontx/upload

Upload a firmware hex file.

**Content-Type**: `multipart/form-data`

**Parameters**:
- `file`: The .hex firmware file

**Response**: `200 OK` with upload confirmation message

### POST /emontx/flash

Program the uploaded firmware to the EmonTX.

**Response**: JSON with result
```json
{
  "success": true/false,
  "code": 0,  // 0 = success, >0 = error code
  "message": "Success"  // Human-readable status
}
```

**Error Codes**:
- 0: Success
- 1: Firmware file not found
- 2: Failed to read firmware file
- 3: Failed to sync with bootloader
- 4: Failed to set flash address
- 5: Failed to write flash page
- 6: Failed to leave programming mode

## Safety Notes

- **Backup**: Always keep a backup of working firmware
- **Power**: Ensure stable power during programming
- **Interruption**: Do not interrupt the programming process
- **Testing**: Test new firmware thoroughly before deployment

## References

- [Optiboot Bootloader](https://github.com/Optiboot/optiboot)
- [STK500 Protocol](https://github.com/Optiboot/optiboot/blob/master/optiboot/bootloaders/optiboot/stk500.h)
- [Intel HEX Format](https://en.wikipedia.org/wiki/Intel_HEX)
- [AVR Bootloaders](https://www.nongnu.org/avr-libc/user-manual/group__avr__boot.html)
