# ESP32 Matter Generic Switch

Firmware for the ESP32-C3 SuperMini accessory in the `sensors` ESP-IDF project. Application v2.1.1 uses ESP-IDF v6.0.2 and managed Espressif Matter 1.6.1.

## Hardware and Matter device

| Function | Configuration |
| --- | --- |
| ESP-IDF project name | `sensors` |
| Button input | GPIO3, active-low, internal pull-up |
| Status LED | GPIO5 |
| Factory-reset button | GPIO9, active-low, internal pull-up; hold 5 seconds then release |
| Matter endpoint 1 | Generic Switch, momentary, two positions |

The v2.1.0 device exposes only application endpoint 1. A press reports position 1; each physical release reports one ShortRelease event and returns the position to 0. It does not classify long presses or multi-press sequences, and it does not wait for a double-click window. The simulated On/Off Light endpoint and its local state toggle are removed.

In Apple Home, configure the Generic Switch's **Single Press** action to toggle the actual lamp: read that lamp's current On/Off state, then command the opposite state. The identified accessories are the momentary button accessory and its target lamp. The owner configured the conditional Single Press shortcut on iPhone/iPad and confirmed that two separate physical presses toggle the real lamp on and off. The inspected Mac Home editor exposed fixed On/Off actions but no shortcut conversion. Legacy automation rules were left untouched.

The existing two-slot OTA partition map is retained. NVS, OTA metadata, secure-certificate storage, and the `fctry` identity partition must be preserved during application updates.

## Build

The supported build pair is ESP-IDF **v6.0.2** and managed `espressif/esp_matter` **1.6.1**. The project manifest and lockfile resolve the managed component; no separate esp-matter checkout is used. See [SETUP.md](SETUP.md).

```sh
source "$HOME/esp/esp-idf-v6.0.2/export.sh"
cd firmware
idf.py -B ../build_sdk6 -D SDKCONFIG="$PWD/sdkconfig.sdk6" -D IDF_TARGET=esp32c3 build
```

## Validation

Run the native LED regression (requires a C compiler):

```sh
tests/run_led_indicator_test.sh
```

The current maintenance update resets LED phase/count for every three-flash sequence and disables the unused diagnostic TestEventTrigger. It preserves the owner-tested momentary-button behavior and commissioning identity.

This retains example Matter commissioning/attestation providers for this prototype. It is not a uniquely provisioned production-device profile. Keep full-flash backups and raw device logs private.

## Wi-Fi and upgrade status

The v2.1.1 maintenance image passed both firmware builds and the LED regression, was flashed application-only with hash verification, and completed a 70-second boot/network observation with both fabrics restored and no Wi-Fi disconnects or unexpected resets. See [the release validation report](docs/validation/2026-10-31-release.md) for evidence and limits.


Saved station credentials and security settings are retained. At startup, the station profile uses all-channel scanning and signal-strength ranking while preserving any explicit BSSID pin. Matter/ESP-IDF manages reconnection. The firmware logs association and disconnection reason/RSSI and samples RSSI, channel, and IPv4 readiness every 30 seconds. Wi-Fi modem sleep remains disabled for this USB-powered button.

The v2.0.0 application-only upgrade is historical validation: it booted on ESP-IDF 6.0.2, restored both Matter fabrics, associated and obtained IPv4, and passed bounded connectivity observations. Apple Home responsiveness was subsequently accepted by the owner. See [the v2.0.0 upgrade record](docs/research/sdk-wifi-upgrade-20261031.md).

The v2.1.0 momentary-only update built successfully and was flashed application-only with hash verification. It booted as 2.1.0, restored both fabrics, obtained IPv4, and established a controller session. The owner confirmed end-to-end lamp toggling in both directions after configuring Single Press. Home had previously shown cached two-service metadata; cosmetic removal of that cached presentation was not independently rechecked. Its private upgrade artifacts and pre-flash backup are kept under `~/.local/share/matter-generic-switch/2026-10-31-momentary-only/`; the earlier v2.0.0 backup remains under `~/.local/share/matter-generic-switch/2026-10-31-upgrade/`.

The QR/manual values were verified at the v2.0.0 boot, and were verified unchanged during the v2.1.0 boot. Confirm the identity from the current boot before commissioning.

## References

- [Matter Generic Switch device type](https://handbook.buildwithmatter.com/howitworks/devicetypes/#generic-switch)
- [Matter specification](https://csa-iot.org/all-solutions/matter/)
- [ESP32-C3 SuperMini reference](https://github.com/sidharthmohannair/Tutorial-ESP32-C3-Super-Mini)
