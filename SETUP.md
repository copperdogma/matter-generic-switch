# ESP32 Matter Generic Switch - Development Environment Setup

## Quick Reference

```bash
# Source environment (required for each terminal session)
source ~/esp/esp-idf/export.sh
source ~/esp/esp-matter/export.sh

# Build and flash for ESP32-C3
cd firmware
idf.py set-target esp32c3
idf.py reconfigure
idf.py build
idf.py -p /dev/tty.usbmodemXXX flash monitor
```

## Prerequisites

Before you begin, ensure your development host (Linux, macOS, or Windows with WSL) meets the prerequisites for ESP-IDF:
- Python 3.8 or newer
- Git
- CMake & Ninja
- Relevant compilers for your OS

Refer to the [ESP-IDF Get Started Guide](https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32c3/get-started/index.html) for detailed, OS-specific instructions.

**ESP32-C3 SuperMini Reference:** [https://github.com/sidharthmohannair/Tutorial-ESP32-C3-Super-Mini](https://github.com/sidharthmohannair/Tutorial-ESP32-C3-Super-Mini)

## 1. Initial Setup

### 1.1 Optional: Clean Previous Installations

If you have existing ESP-IDF/ESP-Matter installations in the `~/esp` directory that you want to remove:

```bash
rm -rf ~/esp
```

### 1.2 ESP-IDF Setup

```bash
mkdir -p ~/esp
cd ~/esp
git clone -b v5.4.1 --recursive https://github.com/espressif/esp-idf.git
cd ~/esp/esp-idf
./install.sh esp32c3
```

### 1.3 ESP-Matter Setup

```bash
cd ~/esp
git clone --recursive https://github.com/espressif/esp-matter.git
cd ~/esp/esp-matter
./install.sh
```

If submodule cloning fails with 502/503 errors:
```bash
git submodule update --init --recursive --force
```

## 2. Environment Management

For each new terminal session, source both environment scripts in this order:

```bash
source ~/esp/esp-idf/export.sh
source ~/esp/esp-matter/export.sh
```

To verify your environment is correctly set up:
```bash
idf.py --version
chip-cert --version
which python3  # Should point to ESP-IDF Python env (~/.espressif/python_env/...)
```

## 3. Project Setup and Configuration

### 3.1 Validate Environment with Example

```bash
cd /path/to/your/projects_directory
mkdir -p matter-generic-switch/firmware-test
cp -r ~/esp/esp-matter/examples/light/* matter-generic-switch/firmware-test/
cd matter-generic-switch/firmware-test
idf.py set-target esp32c3
idf.py build
```

### 3.2 Create the Generic Switch Project

```bash
cd /path/to/your/projects_directory
mkdir -p matter-generic-switch/firmware
cp -r ~/esp/esp-matter/examples/sensors/* matter-generic-switch/firmware/
cd matter-generic-switch/firmware
idf.py set-target esp32c3
```

### 3.3 Configure the Project

Run menuconfig:
```bash
idf.py menuconfig
```

Apply these key settings:

| Section | Setting | Value |
|---------|---------|-------|
| **Component config → CHIP Device Layer → Device Identification Options** | | |
| | Device Vendor Id | 0xFFF1 |
| | Device Product Id | 0x8000 |
| | Default Device Hardware Version | 1 |
| | Device Software Version Number | 1 |
| | Default Device type | 22 (Root Node) |
| **Component config → ESP Matter** | | |
| | Device Info Provider options | Device Info – Custom |
| | Use ESP-Matter Data Model | ✓ (Checked) |
| | Enable Matter Server | ✓ (Checked) |
| | Initialize Thread Stack | ✗ (Unchecked) |

Scroll farther down to **Component config → ESP Matter → Select Supported Matter Clusters** and set:

The menu item lists every cluster that can be compiled into your firmware.  Leave **Basic** and **Identify** enabled (they are mandatory on the root node) and make sure **Switch** and **Switch Events** stay enabled.  You may un-check large, unused clusters (Light, Thermostat, OTA Provider, etc.) to save flash/RAM, but it is optional.

> Device-type assignment (e.g. "Generic Switch") is done entirely in `app_main.cpp` when you call `generic_switch::create(...)`.  The menu does **not** include a "Device Types" page in recent ESP-Matter releases.

Build the project:
```bash
idf.py build
```

## 4. Matter Development Certificates

Create a directory for certificates:
```bash
mkdir -p matter-generic-switch/credentials/dev-certs
cd matter-generic-switch/credentials/dev-certs
```

### 4.1 Generate Certificate Chain

```bash
# Product Attestation Authority (PAA)
chip-cert gen-att-cert --type a -O PAA_key.pem --out PAA_cert.pem \
  --subject-cn "Matter Test PAA" \
  --valid-from "2023-01-01 00:00:00" --lifetime 7305

# Product Attestation Intermediate (PAI)
chip-cert gen-att-cert --type i -O PAI_key.pem --out PAI_cert.pem \
  --subject-cn "Matter Test PAI FFF1" --subject-vid 0xFFF1 \
  --valid-from "2023-01-01 00:00:00" --lifetime 7305 \
  --ca-key PAA_key.pem --ca-cert PAA_cert.pem

# Device Attestation Certificate (DAC)
chip-cert gen-att-cert --type d -O DAC_key.pem --out DAC_cert.pem \
  --subject-cn "Matter Test DAC FFF18000" --subject-vid 0xFFF1 --subject-pid 0x8000 \
  --valid-from "2023-01-01 00:00:00" --lifetime 7305 \
  --ca-key PAI_key.pem --ca-cert PAI_cert.pem
```

### 4.2 Generate Factory NVS Partition

Ensure the esp-matter-mfg-tool is installed:
```bash
python3 -m pip install esp-matter-mfg-tool
```

Generate the partition binary:
```bash
esp-matter-mfg-tool \
  -v 0xFFF1 -p 0x8000 \
  --vendor-name "MyTestVendor" --product-name "GenericSwitch" \
  --hw-ver 1 --hw-ver-str "1.0.0" \
  --mfg-date "2024-05-21" --serial-num "GSWITCH001" \
  --dac-key DAC_key.pem --dac-cert DAC_cert.pem \
  --pai -c PAI_cert.pem \
  --discriminator 3840 --passcode 20202021 \
  --out factory_nvs_output
```

Copy the NVS binary:
```bash
# Find the generated UUID, e.g., d6e078de-1a28-4b57-a713-be172effac1f
cp factory_nvs_output/fff1_8000/{UUID}/{UUID}-partition.bin mfg_nvs.bin
```

## 5. Flashing Firmware and NVS Partition

1. **Flash the Factory NVS Partition:**
   ```bash
   cd /path/to/your/project/firmware
   python -m esptool --chip esp32c3 -p /dev/tty.usbmodemXXX write_flash 0x10000 ../credentials/dev-certs/mfg_nvs.bin
   ```

2. **Build and Flash Main Application:**
   ```bash
   idf.py build
   idf.py -p /dev/tty.usbmodemXXX flash monitor
   ```

3. **Commissioning Information:**
   After flashing, check the serial monitor for:
   - QR code payload (e.g., `MT:Y.K90GSY00KA0648G00`) 
   - Manual pairing code (e.g., `34970112332`)

**Bootloader Mode:** The ESP32-C3 SuperMini might require being put into bootloader mode. Typically, this involves holding down the `BOOT` button (often GPIO9), pressing and releasing the `RESET` (or `EN`) button, and then releasing the `BOOT` button.

## 6. Troubleshooting

| Issue | Solution |
|-------|----------|
| **"gn command not found"** | Verify environment: `echo $ESP_MATTER_PATH` <br>Check for executable: `ls $ESP_MATTER_PATH/connectedhomeip/connectedhomeip/.environment/cipd/packages/pigweed/gn` |
| **Python environment issues** | Ensure using ESP-IDF Python: `which python3` <br>Re-run ESP-IDF's `install.sh` if needed |
| **Submodule cloning failures** | Use force flag: `git submodule update --init --recursive --force` |
| **Build errors in C++ code** | Try full clean: `idf.py fullclean` before rebuilding |
| **Serial Port Not Found** | Double-check the serial port name (e.g., `/dev/tty.usbmodemXXXX`). Ensure you have the necessary USB drivers (e.g., CH340/CH341 or CP210x) |
| **Build Errors** | Carefully read the error messages. They often point to configuration issues in `sdkconfig` or problems in the code |
| **Flashing Fails** | Ensure the board is in bootloader mode. Check the USB cable and connection. Try a different USB port |
| **Commissioning failures** | Factory reset: Hold BOOT button (GPIO 9) for 5+ seconds, then release <br>Remove device from Matter controller app <br>QR codes remain the same after factory reset (correct behavior) |

## Primary Reference Documents
- [ESP-Matter Programming Guide](https://docs.espressif.com/projects/esp-matter/en/latest/esp32/developing.html)
- [ESP-IDF Get Started](https://docs.espressif.com/projects/esp-idf/en/v5.4.1/esp32c3/get-started/index.html)
