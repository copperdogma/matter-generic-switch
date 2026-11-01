# Changelog

2026-10-31 — application 2.1.1
- Fix LED sequence phase leaking between presses; exactly three pulses now complete off, including after a restart. Add a native regression compiling the real driver with timer/LEDC fakes.
- Disable unused Matter diagnostic TestEventTrigger while retaining the existing commissioning identity/provider profile and momentary-button behavior.
- Validate both the existing board profile and a fresh configuration generated from tracked defaults. The application-only image is flashed and passed a 70-second boot/network observation; see [release validation](validation/2026-10-31-release.md).

2026-10-31 — application 2.1.0 (owner-accepted)
- Reduce the Matter device to endpoint 1, a momentary Generic Switch with MomentarySwitch and MomentarySwitchRelease only. Every physical release produces one ShortRelease event and returns CurrentPosition to 0; do not classify long or multi-press gestures or wait for a double-click window.
- Remove the simulated On/Off Light endpoint 2 and its local state toggle. The momentary button Single Press reads the real target lamp state and commands the opposite state. The owner confirmed physical presses toggle the lamp both ways; legacy automation rules remain untouched.
- Keep ESP-IDF 6.0.2, managed ESP-Matter 1.6.1, Wi-Fi behavior/configuration, two-slot partition map, and identity/provider profile unchanged.
- Build and hash-verified application-only flash passed. A 180-second boot capture restored both fabrics and showed no post-boot resets or Wi-Fi disconnects. The owner accepted end-to-end physical lamp toggling; cached Home presentation of the removed service was not independently rechecked. The 2.0.0 evidence below remains historical.
- Keep the private v2.1.0 backup/artifacts under `~/.local/share/matter-generic-switch/2026-10-31-momentary-only/`; the earlier v2.0.0 backup remains under `~/.local/share/matter-generic-switch/2026-10-31-upgrade/`.

2026-10-31 — application 2.0.0
- Pin ESP-IDF 6.0.2 and managed ESP-Matter 1.6.1; track the resolved component lock.
- Adapt onboarding-code headers and explicitly declare supported Switch features for the new data model.
- Correct momentary position/release handling and add single/multiple-press completion reporting.
- Add bounded-rate Wi-Fi signal/IP state and disconnect-reason diagnostics; retain SDK-owned reconnection. Upgrade retained station profiles to all-channel scanning with signal-strength ranking while preserving credentials/security settings.
- Keep project name, endpoints, partition map, credentials and identity-provider profile. Replace obsolete setup instructions with application-only updates and a private backup procedure.
- Application-only flash and 100-second network observation passed; six probes replied with no packet loss. Disable modem sleep for this USB-powered button. Owner confirmed final Apple Home responsiveness; widened double-click physical verification remains pending; see docs/research/sdk-wifi-upgrade-20261031.md.

20250509: Initial Matter Setup
- Set up computer environment:
  brew install cmake ninja dfu-util
  python --version (shows me running 3.11.5 which is >=3 which is ok)
  brew install ccache
- Followed SETUP.md instructions to the letter.
  [2025-05-10 01:18:38,882] [   INFO] - Generated onboarding data and QR Code



20250117: Initial Hardware Validation
- Completed PIR sensor testing:
  - Verified 0V/3.3V output levels
  - Confirmed 2.5s block time behavior
  - Documented signal characteristics
- Power system validation:
  - Tested ESP32 USB-C power
  - Verified 5V output capability
  - Documented power budget analysis
- Documentation updates:
  - Located and archived ESP32-S3 documentation (migrated to ESP32-C3)
  - Added detailed PIR test results
  - Consolidated power management documentation
  - Organized reference materials

20250117: Hardware Specification Updates
- Verified HC-SR501 PIR sensor capabilities
- Updated documentation to reflect hardware capabilities

20250116: Initial Project Setup
- Created project documentation structure
- Defined core requirements and specifications
- Established technical design approach
- Selected key components:
  * ESP32-C3 SuperMini as main controller
  * HC-SR501 PIR sensor for motion detection
  * Matter protocol for HomeKit integration
  * USB-C power delivery

Build notes:
- Researched ESP32 Matter implementation options
- Evaluated PIR sensor configurations for 45' range
- Investigated power delivery requirements
- Key decisions:
  * ESP-IDF v5.1+ required for Matter support
  * 50ms debouncing for motion detection
  * 5-minute occupancy timeout
  * Dual USB-C ports for power/programming
- Referenced ESP32 Motion Sensor Tutorial and Matter documentation
- Created initial documentation structure:
  * spec.md: Requirements and success criteria
  * design.md: Technical implementation details
  * todo.md: Active tasks and planning
  * CHANGELOG.md: Project history
