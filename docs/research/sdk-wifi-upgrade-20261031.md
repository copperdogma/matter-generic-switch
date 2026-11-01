# SDK and Wi-Fi platform selection — 2026-10-31

## Decision and sources

Use exact ESP-IDF 6.0.2 and managed `espressif/esp_matter` 1.6.1, with the resolved component lock tracked. [The component README](https://components.espressif.com/components/espressif/esp_matter/versions/1.6.1/readme) recommends this IDF version and identifies compatible connectedhomeip revision b1902b39ac. [Espressif's supported-version table](https://github.com/espressif/esp-matter#supported-matter-specification-versions) distinguishes the stable 1.6.1 branch from ongoing 1.7 development. Do not independently replace CHIP or patch managed dependencies.

A recent sibling project had already built and board-tested this pair. Its identity providers and larger single-application partition layout were not reused: this switch retains its original example-provider profile and two OTA slots.

## Migration requirements demonstrated locally

The attached target was an ESP32-C3 rev 0.4 with 4 MB embedded flash, running application 1.0 on IDF 5.4.1. Its partition table matched `firmware/partitions.csv`. A full-flash backup was retained privately before each substantive migration; updates wrote only the application at 0x20000.

- The pairing-code utility moved to `setup_payload/OnboardingCodesUtil.h`.
- Matter 1.6.1 requires explicit Switch feature flags. An initial image built but asserted at cluster creation when neither Momentary nor Latching was selected. Installed `esp_matter_cluster.cpp` validates this conformance rule; the corrected configuration passed boot.
- The new Switch server owns CurrentPosition. The legacy attribute writer returned ESP_ERR_NOT_SUPPORTED during physical testing. Use `Switch::FindClusterOnEndpoint(...)->SetCurrentPosition(...)` under the Matter lock, as declared by the pinned SDK's `SwitchCluster.h`.
- Preserve existing credentials/fabrics and provider selection. Never erase NVS as automatic initialization recovery.

## Wi-Fi experiments

Problem classes: selecting an access point among multiple matches, authentication/association failures, and packet delivery after association. The original firmware repeatedly retried association and eventually reported a weak signal. An SDK upgrade alone did not resolve it: a 90-second upgraded capture still had nine disconnects and no IPv4 lease, despite eventual association.

[Espressif's Wi-Fi overview](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/wifi-driver/overview.html) documents all-channel scanning with signal-strength sorting. The pinned CHIP `NetworkCommissioningDriver.cpp` and `ESP32Utils.cpp` apply this policy when new/default credentials are supplied, but do not rewrite an existing saved fast-scan profile. Initialize the standard IDF driver before Matter starts it, copy the saved station configuration, and change only scan/sort policy. Preserve credentials, security settings and any explicit BSSID pin. The pinned `ESP32Utils::InitWiFiStack` supports an already initialized driver/netif. Matter continues to own connection/reconnection; no competing reconnect loop is added.

An all-channel test associated on the first candidate attempt, obtained IPv4 at approximately 11 seconds, and later established a CASE session with report acknowledgements. A subsequent reboot still retried authentication and had poor packet delivery despite a good reported signal. Thus an IP lease/RSSI alone is insufficient acceptance.

[Espressif's modem-sleep guidance](https://docs.espressif.com/projects/esp-idf/en/latest/esp32c6/api-guides/wifi-driver/wifi-performance-and-power-save.html) describes the latency/power tradeoff of WIFI_PS_NONE. The corresponding API was checked in installed IDF 6.0.2 for the C3. This continuously USB-powered accessory disables modem sleep. The bounded comparison then obtained IPv4 at about 11 seconds, received all six ICMP replies and had no disconnects during 100 seconds. Packet timing remained variable; RF conditions varied between runs, so this does not prove sleep was the sole cause or establish long-term reliability.

## Identity and qualification boundary

Existing commissioning codes and both Matter fabrics survived. The retained example identity/test commissioning profile is for a development/prototype device, not unique production provisioning. Creating a production identity is a separate commissioning migration. The unused diagnostic TestEventTrigger is disabled in the validated follow-up.

Raw serial logs, device identifiers, network addresses, full flash images and sdkconfig snapshots stay outside Git in private local recovery storage. Same-host recovery copies are not off-host backups. The subsequent momentary-only behavior and owner acceptance are recorded in [the button migration note](momentary-lamp-toggle-20261031.md).
