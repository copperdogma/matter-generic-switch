# ESP32 Matter Generic Switch

A Matter 1.4–compatible *Generic Switch* built with the ESP32-C3 SuperMini and a single momentary push-button on GPIO 3. The device exposes the **Generic Switch** device type defined in the Matter 1.4 specification [“Switches and Controls → Generic Switch”](https://handbook.buildwithmatter.com/howitworks/devicetypes/#generic-switch) and integrates seamlessly with Apple Home, Google Home, and Amazon Alexa.

## Quick Reference

```bash
# Build and flash
cd firmware
source ~/esp/esp-idf/export.sh
source ~/esp/esp-matter/export.sh
idf.py build
idf.py -p /dev/tty.usbmodemXXX flash monitor

# Current commissioning codes
QR Code: MT:Y.K90GSY00KA0648G00
Manual Code: 34970112332
https://project-chip.github.io/connectedhomeip/qrcode.html?data=MT%3AY.K90GSY00KA0648G00
```

## Features

- **Momentary Push-Button:** Maps to *Generic Switch* endpoint 1, press and release events are reported via the Switch Events cluster.
- **Matter 1.4 Protocol:** Works with all major smart-home controllers (Apple Home, Google Home, Amazon Alexa).
- **Secure Commissioning:** On-device QR code and manual pairing code for easy setup.
- **Power:** USB-C powered for continuous operation.
- **LED Status (optional):** Visual feedback for device status (e.g., power, button press, commissioning).

## Hardware Components

| Component | Details |
|-----------|---------|
| **ESP32-C3 SuperMini** | Single-core 160 MHz, 400KB SRAM, 4MB Flash, Wi-Fi, BLE 5.0 |
| **Momentary Push-Button** | Normally-open push-button, debounced in software |

## Hardware Connections

### ESP32-C3 SuperMini Connections

| ESP32-C3 Pin | Component Pin | Notes                   |
|--------------|---------------|-------------------------|
| GND          | Button GND    |                         |
| 5V/3.3V      | Button VCC    |                         |
| GPIO 3       | Switch Signal | Active-high when button pressed |
| GPIO 5       | LED Anode (+) | Through 220 Ω resistor   (optional) |
| GPIO 9       | BOOT Button   | Factory reset (built-in) |
| GND          | LED Cathode (-)|                         |

*For a detailed wiring diagram, see [docs/circuit_diagram.md](docs/circuit_diagram.md)*

## Setup & Development

1. **Environment Setup**
   - Follow instructions in [SETUP.md](SETUP.md) to set up the development environment, ESP-IDF, ESP-Matter SDK, and development certificates.

2. **Build & Flash**
   ```bash
   cd firmware
   source ~/esp/esp-idf/export.sh
   source ~/esp/esp-matter/export.sh
   # Build for ESP32-C3
   idf.py set-target esp32c3
   idf.py reconfigure
   idf.py build
   idf.py -p PORT flash monitor
   ```

3. **Commissioning**
   - After flashing, check serial monitor for QR code and pairing code
   - Use Apple Home or other Matter controller to add the device
   - Current codes: QR `MT:Y.K90GSY00KA0648G00`, Manual `34970112332`

## Project Structure

```
matter-generic-switch/
├── credentials/              # Certificates and factory data
├── docs/                     # Documentation
├── firmware/                 # ESP32 firmware source code
│   ├── main/                 # Main application logic
│   └── CMakeLists.txt        # Main CMake file
├── README.md                 # This file
└── SETUP.md                  # Environment setup guide
```

## Key Implementation Notes

### Default Configuration
The Generic Switch exposes a single stateless switch on endpoint 1 with the following default configuration:
* **Switch Type:** Momentary
* **Number of Positions:** 2 (pressed / released)
* **Current Position:** 0 (released)
* **Switch Latency:** < 10 ms (software debounce)

### Commissioning Tips
1. **Always use the current QR/manual codes** printed to the serial monitor after flashing.
2. **Factory reset:** Hold the BOOT button (GPIO 9) for 5+ seconds, then release.
3. **If commissioning fails:**
   - Remove the device from the controller app.
   - Perform a factory reset.
   - Re-commission using the same codes (QR codes don't change after factory reset).

### Apple Home Integration Notes
* Apple Home may briefly display "No Code Needed" if the device was recently removed and re-added.
* **UI quirk:** as of iOS 17 Apple Home hasn't added a dedicated tile for the Matter *Generic Switch* device-type. The accessory therefore appears with the generic name "Accessory" and a grey icon. Automations still receive the correct *Switch Events* (e.g. "Button 1 Pressed", "Released").
* Switch Events appear in automations as "Button 1 Pressed" / "Released."

### Factory Reset

**When to use factory reset:**
- Device shows "already added to another home" error
- Commissioning fails repeatedly
- Need to clear all pairing data and start fresh

**How to perform factory reset:**
1. **Button method:** Hold the BOOT button (GPIO 9) for 5+ seconds, then release
2. **Console method:** In serial monitor, type `factory_reset confirm` and press Enter

**What factory reset does:**
- Clears all pairing data and fabric information
- Clears Wi-Fi credentials
- **Keeps the same QR/manual codes** (this is correct behavior)
- Device will restart and be ready for new commissioning

### Known Limitations

1. Some non-critical console warnings appear during operation but don't affect functionality.
2. The device has been primarily tested with Apple Home; integration with other Matter controllers may require additional validation.

## Technical Specifications

- **Matter Version:** 1.4 (Universal smart home compatibility)
- **Microcontroller:** ESP32-C3 SuperMini (160 MHz, 400KB SRAM, 4MB Flash)
- **Input:** Single momentary push-button (GPIO 3)
- **Connectivity:** Wi-Fi 802.11 b/g/n, Bluetooth 5.0 LE
- **Development:** ESP-IDF v5.4.1, ESP-Matter SDK

## Troubleshooting

### Common Issues

**"Already added to another home" error:**
- Perform factory reset: Hold BOOT button (GPIO 9) for 5+ seconds, then release
- Remove device from Apple Home completely
- Re-add using the same QR code (codes don't change after factory reset)

**Button not responding:**
- Ensure you're using GPIO 9 (BOOT button), not GPIO 0
- Check button wiring and connections
- Verify firmware is flashed correctly

**Commissioning fails:**
- Check Wi-Fi credentials and network connectivity
- Ensure device is within range of your router
- Try factory reset and re-commission

**Serial monitor not connecting:**
- Check USB cable and port
- Kill any existing monitor processes: `pkill -f monitor`
- Try different USB port

## References

- **ESP32-C3 SuperMini:** [Pinout and specifications](https://github.com/sidharthmohannair/Tutorial-ESP32-C3-Super-Mini)
- **Matter Protocol:** [Matter 1.4 Specification](https://csa-iot.org/all-solutions/matter/) - see also the [Generic Switch device type](https://handbook.buildwithmatter.com/howitworks/devicetypes/#generic-switch)

## Author

Cam Marsollier
