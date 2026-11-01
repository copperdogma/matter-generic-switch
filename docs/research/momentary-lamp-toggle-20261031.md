# Momentary button and lamp toggle — 2026-10-31

## Contract

The physical button is an event source; Apple Home owns the actual lamp state. Application 2.1.0 removes the simulated On/Off Light endpoint and retains Generic Switch application endpoint 1. Mandatory root endpoint 0 remains.

The Switch feature map is MomentarySwitch | MomentarySwitchRelease (0x06). Press-down sets native CurrentPosition to 1 and emits InitialPress. Every physical release sets position 0 and emits one ShortRelease. Long/multi-press classification is disabled; there is no double-click waiting period. Two separate physical actuations are two independent requests. Register input after Matter startup, when the native server exists.

The pinned button 4.2.3 implementation was inspected: BUTTON_PRESS_UP fires immediately in short, repeated and long-hold release paths, independently of delayed click-classification callbacks. This supports the simple two-callback mapping without a new gesture recognizer.

## Sources and reuse boundaries

- [Espressif Generic Switch](https://docs.espressif.com/projects/arduino-esp32/en/latest/matter/ep_generic_switch.html) documents the simple momentary/release combination and Apple Home automation support. Reuse protocol behavior; this firmware uses native ESP-IDF/Matter rather than Arduino.
- [Apple conditional home automation guidance](https://support.apple.com/en-mide/guide/shortcuts/apd2a290f633/ios) documents Convert To Shortcut on iPhone/iPad.
- [Apple's Mac Home guide](https://support.apple.com/en-lamr/guide/home/hme33ae10cb7/mac) describes accessory actions. Local inspection, rather than an assumption about all Mac versions, established that this Mac editor offered fixed On/Off actions but no shortcut conversion. An unsaved fixed-state draft was cancelled.

## Home action

Configure the button's Single Press action on iPhone/iPad:

1. Convert to Shortcut.
2. If the **actual lamp** is on, set that lamp off.
3. Otherwise, set that lamp on.
4. Save, then test separate physical presses with the lamp initially off and initially on.

Do not infer the lamp state from the ESP32 or the removed endpoint. Existing legacy automation rules were left untouched. No factory reset or re-pair was used to address cached metadata.

## Validation

Application 2.1.0 built and flashed application-only with esptool hash verification. Its generated partition table matched the full pre-update backup byte-for-byte. The original bootloader, partition table, NVS, factory identity and OTA metadata were retained.

A 180-second capture identified 2.1.0 / IDF 6.0.2, restored both fabrics and the original commissioning codes, obtained IPv4 around 12 seconds and established a CASE session around 46 seconds. It had no post-boot resets or Wi-Fi disconnects. Three ICMP probes eventually replied; one exceeded the 1-second wait, so uniformly low latency is not claimed.

The owner subsequently saved the conditional action and confirmed separate physical presses toggle the real lamp in both directions. This is end-to-end owner acceptance on the flashed image. The capture itself contained no presses, so it does not provide per-gesture native event counts. Home had shown cached two-service metadata; cosmetic removal of that presentation was not subsequently rechecked.

Application SHA-256: `5213264626b4e02e00b73e822b82c0d1255261e9de5c5cd5746ad2576aecba6a`.

## Follow-up validation

The check-in review found a retained LED timer phase that could shorten later blink sequences. The follow-up uses explicit per-sequence phase reset and counts completed off transitions. Installed IDF 6.0.2 `esp_timer.h` documents serialized task callbacks; button and blink callbacks use that timer task. Regression checks compile the actual LED driver with fake timer/LEDC APIs and cover repeated sequences, interruption/restart, and explicit cancellation. Hardware timing/electrical behavior is outside that host-test boundary.
