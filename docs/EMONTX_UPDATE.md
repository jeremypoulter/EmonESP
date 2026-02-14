# EmonTX Firmware Update Feature

## Overview

The EmonESP now includes the ability to update the firmware on connected EmonTX devices (ATmega328-based) over WiFi. This feature uses the ESP8266AVRISP library to implement the STK500 programming protocol over TCP/IP.

## Hardware Requirements

- EmonESP device (ESP8266-based)
- EmonTX device (ATmega328-based) connected via SPI
- Proper wiring between ESP8266 and EmonTX:
  - GPIO12 → MISO
  - GPIO13 → MOSI
  - GPIO14 → SCK
  - GPIO4 (default) → RESET

**Important:** If the EmonTX runs at 5V and ESP8266 at 3.3V, you **must use level shifters** to protect the ESP8266 from damage.

## Configuration

The reset pin can be configured by defining `EMONTX_RESET_PIN` in your build flags if you need to use a different GPIO pin:

```ini
build_flags = 
  -DEMONTX_RESET_PIN=5  ; Use GPIO5 instead of default GPIO4
```

## Usage

### Programming via avrdude

Once the EmonESP is connected to your WiFi network, you can program the connected EmonTX using avrdude from your computer:

```bash
avrdude -c arduino -p m328p -P net:<ESP_IP_ADDRESS>:328 -U flash:w:firmware.hex:i
```

Replace `<ESP_IP_ADDRESS>` with the IP address of your EmonESP device.

### Checking Programmer Status

You can check the programmer status via HTTP:

```bash
curl http://<ESP_IP_ADDRESS>/emontx/programmer
```

Response example:
```json
{
  "available": 1,
  "state": "idle",
  "port": 328,
  "reset_pin": 4,
  "avrdude_command": "avrdude -c arduino -p m328p -P net:192.168.1.100:328 -U flash:w:firmware.hex:i"
}
```

The `/status` endpoint also includes programmer information:
```bash
curl http://<ESP_IP_ADDRESS>/status
```

### mDNS Discovery

The programmer registers itself as an avrisp service via mDNS, making it discoverable on the local network.

## Programming States

The programmer has three states:

1. **idle**: No active programming session, EmonTX is running normally
2. **pending**: TCP connection established, waiting for programming commands
3. **active**: Actively programming the EmonTX

During programming, the EmonTX will be held in reset and will not be able to send data.

## Troubleshooting

### Connection Issues

- Ensure the ESP8266 and your computer are on the same network
- Check firewall settings allow TCP port 328
- Verify the EmonTX reset line is properly connected

### Programming Errors

- If you get "programmer not responding" errors, check the SPI wiring
- Ensure proper level shifting if voltage levels differ
- Try lowering the avrdude baud rate: add `-b 115200` to the avrdude command

### Wiring Issues

- Double-check all SPI connections (MISO, MOSI, SCK, RESET)
- Ensure ground is common between ESP8266 and EmonTX
- Verify the reset pin configuration matches your hardware

## Technical Details

- **Protocol**: STK500 over TCP/IP (port 328)
- **SPI Frequency**: 300 kHz (safe default)
- **Library**: ESP8266AVRISP (built into ESP8266 Arduino core)
- **Reset Logic**: Active-low by default (configurable)

## Security Considerations

The avrisp service is exposed on the network without authentication. If this is a concern:

1. Use network segmentation to isolate IoT devices
2. Use firewall rules to restrict access to port 328
3. Enable WiFi encryption (WPA2/WPA3)

## Example Programming Session

```bash
# 1. Check that the programmer is available
curl http://emonesp.local/emontx/programmer

# 2. Upload new firmware to EmonTX
avrdude -c arduino -p m328p -P net:emonesp.local:328 -U flash:w:emontx_firmware.hex:i

# 3. Verify the upload was successful
# The EmonTX should start running the new firmware immediately after programming
```

## References

- [ESP8266AVRISP Library](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266AVRISP)
- [avrdude Documentation](https://www.nongnu.org/avrdude/)
- [STK500 Protocol](http://www.atmel.com/images/doc0943.pdf)
