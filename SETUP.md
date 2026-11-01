# ESP32 Matter Generic Switch — build and update

## Pinned platform

The `sensors` ESP-IDF project targets the ESP32-C3. Application v2.1.1 uses:

- ESP-IDF **v6.0.2**
- managed `espressif/esp_matter` **1.6.1**, resolved by `firmware/main/idf_component.yml` and `firmware/dependencies.lock`

Use the versioned ESP-IDF export script. Do not source a separate esp-matter environment or replace this project's example identity providers with another project's factory profile.

```sh
source "$HOME/esp/esp-idf-v6.0.2/export.sh"
idf.py --version
```

Confirm the reported version is ESP-IDF v6.0.2. If needed, install it in a separate versioned directory using Espressif's instructions. Do not delete or replace `~/esp` or another existing SDK installation.

## Build

From the repository root:

```sh
source "$HOME/esp/esp-idf-v6.0.2/export.sh"
cd firmware
idf.py -B ../build_sdk6 -D SDKCONFIG="$PWD/sdkconfig.sdk6" -D IDF_TARGET=esp32c3 build
```

The absolute `SDKCONFIG` path avoids resolving the file relative to the separate build directory. Keep the build directory and sdkconfig isolated from older SDK builds. Use the tracked managed-component lockfile; do not independently update CHIP or use a mutable esp-matter branch.

## v2.1.0 device behavior

The v2.1.0 Matter device has only application endpoint 1, a momentary Generic Switch with the MomentarySwitch and MomentarySwitchRelease features. It has two positions: a press sets position 1; every physical release emits one ShortRelease event and sets position 0. Long-press and multi-press classification are not enabled, and the application does not wait for a double-click window. The simulated On/Off Light endpoint 2 and local On/Off toggle are removed.

For the Apple Home **Single Press** action, toggle the actual lamp by reading that lamp's current On/Off state and commanding its opposite. The targets are the momentary button accessory and its target lamp. The owner configured the conditional Single Press shortcut on iPhone/iPad and confirmed physical presses toggle the lamp in both directions; the inspected Mac editor exposes fixed On/Off actions but no shortcut conversion. Legacy automation rules were left untouched; the new Single Press action has owner acceptance.

Hardware and identity configuration to preserve:

- Project name `sensors`; GPIO3 button is active-low with internal pull-up.
- GPIO5 status LED; GPIO9 factory-reset button is active-low with internal pull-up (hold 5 seconds, then release).
- Keep the current Wi-Fi policy/configuration and the two-slot OTA map (`ota_0`, `ota_1`).
- Preserve NVS, OTA metadata, secure-certificate storage, and `fctry` identity partition.
- Retain this project's example DAC, commissionable-data, and device-instance-info providers. Do not import the sibling occupancy project's factory-data/provider profile.

## Safe application update

The v2.0.0 private upgrade backup remains at:

```text
~/.local/share/matter-generic-switch/2026-10-31-upgrade/
```

The current v2.1.0 private artifacts and pre-flash backup are kept separately at:

```text
~/.local/share/matter-generic-switch/2026-10-31-momentary-only/
```

Before writing firmware, confirm the connected device is the intended ESP32-C3 and compare its partition table with `firmware/partitions.csv`. Inspect the generated table and `../build_sdk6/flash_args`; verify application slots and retained data partitions match. Stop if the map differs. Keep both backup directories private because their contents can include device-specific data.

Flash the application only, substituting the serial port identified on the current machine:

```sh
idf.py -B ../build_sdk6 -D SDKCONFIG="$PWD/sdkconfig.sdk6" -D IDF_TARGET=esp32c3 \
  -p /dev/cu.usbmodemXXXX app-flash monitor
```

Do not use `flash`, `erase-flash`, raw `write-flash`, factory-NVS generation, or commands that write the partition table, NVS, secure-certificate, OTA metadata, or `fctry` partitions. Application-only flashing preserves commissioning and identity state.

## Validation status

The v2.1.1 maintenance image passed both firmware builds and the LED regression, was flashed application-only with hash verification, and completed a 70-second boot/network observation with both fabrics restored and no Wi-Fi disconnects or unexpected resets. See [the release validation report](docs/validation/2026-10-31-release.md) for evidence and limits.


Run `tests/run_led_indicator_test.sh` with a host C compiler to exercise the actual LED driver against timer/LEDC fakes. The check covers repeat sequences, interrupted restart, off cancellation, exact pulse count and timer termination. This does not validate GPIO electrical timing.

The retained example commissioning/attestation providers are a development/prototype identity. Do not treat the image as uniquely provisioned production firmware. The unused diagnostic TestEventTrigger is disabled in v2.1.1; commissioning/profile changes require their own planned migration.

The v2.0.0 SDK upgrade is historical validation only. It booted as application 2.0.0 on ESP-IDF 6.0.2, restored two Matter fabrics, obtained IPv4, and passed bounded network observations; Apple Home responsiveness was subsequently owner-accepted. Details and limits are in [the v2.0.0 report](docs/research/sdk-wifi-upgrade-20261031.md).

The v2.1.0 build and application-only flash passed; boot restored both fabrics, obtained IPv4, and established a controller session. The owner confirmed two separate physical presses toggle the real target lamp in both directions. Cached Home presentation of the removed service was not independently rechecked. Do not describe the v2.0.0 results as v2.1.0 validation. Wi-Fi scanning/reconnection policy, modem-sleep configuration, and partition map remain unchanged from v2.0.0.

The QR/manual values were checked against the v2.0.0 boot. Both codes were verified unchanged during the v2.1.0 boot. Verify the current device identity before commissioning. The new state-aware Single Press action is owner-verified; legacy automation rules were not removed.

## References

- [ESP-IDF v6.0.2 documentation](https://docs.espressif.com/projects/esp-idf/en/v6.0.2/esp32c3/)
- [ESP-Matter programming guide](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html)
- [ESP32-C3 SuperMini reference](https://github.com/sidharthmohannair/Tutorial-ESP32-C3-Super-Mini)
