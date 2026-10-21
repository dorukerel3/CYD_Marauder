# CYD_Marauder

A complete firmware and hardware project for the dual-USB **"Cheap Yellow Display" (CYD2USB / ESP32-2432S028R)**: a substantially reworked build of **[ESP32 Marauder](https://github.com/justcallmekoko/ESP32Marauder)** with a rebuilt touch interface, bidirectional GPS, an offline SD map viewer, battery telemetry, a Wi-Fi file server, over-the-air updates, and a seven-language menu, accompanied by a custom u-blox **NEO-F10N** GPS daughterboard designed from scratch in KiCad. This repository is intended to be self-sufficient: it carries the full sketch, every library the build depends on, the fabrication files for the GPS board, and a prebuilt single-file flash image, so the exact binary can be reproduced with one command.

> **For authorized security testing and education only.** ESP32 Marauder is a Wi-Fi and Bluetooth analysis instrument. Confine its use to networks and devices you own or are expressly permitted to assess, and remain accountable to the law in your jurisdiction.

---

## Table of Contents

- [Project Overview](#project-overview)
- [Key Features](#key-features)
- [Hardware](#hardware)
- [Repository Contents](#repository-contents)
- [Flashing the Prebuilt Release](#flashing-the-prebuilt-release)
- [Building the Firmware from Source](#building-the-firmware-from-source)
- [GPS Add-on PCB](#gps-add-on-pcb)
- [Enclosures](#enclosures)
- [Wi-Fi Over-the-Air Updates](#wi-fi-over-the-air-updates)
- [Author and License](#author-and-license)

---

## Project Overview

The firmware targets the two-USB revision of the Cheap Yellow Display, a low-cost ESP32 board pairing a WROOM-32U module (external u.FL antenna) with a 2.8 inch ST7789 touch panel. The stock Marauder experience has been reshaped around that hardware: colours render correctly on the inverted 2-USB panel, the menu is paginated into large finger-friendly buttons with corner-based back navigation, and the interface label set is translatable into seven languages whose selection persists across reboots in non-volatile storage.

Beyond the interface, the build adds a layer of genuinely useful field tooling. A streamed PNG tile renderer draws OpenStreetMap raster tiles straight off the SD card, so dense city maps display without a large RAM copy, overlaid with a live GPS marker, a follow mode, and recallable favourites. A bidirectional GPS link reports its negotiated baud rate and measured fix rate on screen. A MAX17043-class fuel gauge contributes a battery percentage and icon to the status bar. The device can also serve its SD card over HTTP and FTP, and it can update itself over Wi-Fi by pulling the latest release of this repository.

Every pin assignment, partition choice, library version, and linker flag is fixed in `build.sh`, which is what allows a clone of this repository to regenerate the published image byte for byte.

---

## Key Features

| Capability | Detail |
|---|---|
| Display | Correct colour rendering on the inverted CYD2USB ST7789 panel, with XPT2046 resistive touch tuned on a pressure threshold rather than a timeout |
| Interface | Paginated touch menus, corner back navigation, and a seven-language label set (English, Japanese rōmaji, German, French, Italian, Portuguese, Turkish) saved to NVS |
| Offline maps | OpenStreetMap raster tiles streamed from SD (`/maps/<z>/<x>/<y>png.tile`) via PNGdec file callbacks, with pan, zoom between present tile levels, and a centre crosshair |
| GPS | Bidirectional link on IO35 (RX) and IO17 (TX), runtime-selectable pins and baud, a live position marker with follow mode, and on-screen readout of the active **baud** and measured **fix rate (Hz)** |
| Battery | MAX17043 or MAX17048 fuel gauge over I²C, auto-detected at address `0x36`, shown as a percentage and icon in the status bar |
| Places | Save and recall named locations from the map top bar, persisted to `/maps/favs.txt` on the SD card |
| File server | Joins one of your own networks when a matching credential file is in range, otherwise self-hosts an access point with on-screen approval, serving the SD card over HTTP and FTP |
| Over-the-air updates | Pulls the newest GitHub release asset over HTTPS and flashes the spare OTA slot, transparently handling a merged image by skipping its first `0x10000` |

---

## Hardware

| Aspect | Part or Value |
|---|---|
| Board | ESP32-2432S028R "Cheap Yellow Display", two-USB variant (USB-C and micro-USB side by side) |
| Module | ESP32-WROOM-32U with external antenna (attach a 2.4 GHz antenna to the u.FL/IPEX connector before any Wi-Fi work) |
| Display | 2.8 inch ST7789, 320 by 240, inverted panel |
| Touch | XPT2046 resistive controller over SPI |
| GPS | u-blox NEO-F10N on the custom daughterboard; TX to ESP RX on **IO35**, ESP TX to GPS RX on **IO17** |
| Fuel gauge | MAX17043 or MAX17048 over I²C, **SDA on IO22**, **SCL on IO27** |
| Programming | Flash over the **micro-USB** port; the USB-C port lacks CC resistors and often will not enumerate on a C-to-C cable |

---

## Repository Contents

| Path | Contents |
|---|---|
| `source/esp32_marauder/` | The Arduino sketch (the firmware itself) |
| `source/libs/` | Every library the build requires, pinned to the exact versions used (the build consumes these, not upstream copies) |
| `compile/` | A self-contained mirror of the sketch and libraries, convenient for building in a single directory |
| `build.sh` | The one-command build: toolchain install, compilation, and merge into a single flashable image |
| `marauder_cyd_v1_0x0.bin` | The prebuilt merged image, ready to flash at offset `0x0` |
| `GPS-NEO_F10N-00B_Files/` | The custom NEO-F10N GPS daughterboard: KiCad project, Gerbers, and drill files |


---

## Flashing the Prebuilt Release

The flashable artefact is **`marauder_cyd_v1_0x0.bin`**, a single merged image that combines the bootloader, partition table, `boot_app0`, and application, and writes as one file at offset `0x0`. Connect the board through its **micro-USB** port before flashing.

### Option A: Browser flasher (no installation)

Open the web flasher at **[dorukerel.com](https://dorukerel.com)** in Chrome or Edge (both expose the WebSerial API), connect the board over micro-USB, choose the serial port, and flash `marauder_cyd_v1_0x0.bin` at offset `0x0`. No Python, drivers, or command line are required.

Should the connection fail, place the board into download mode first by holding **BOOT**, tapping **RST**, then releasing **BOOT**, and retry.

### Option B: Command line with esptool

This path requires Python together with [esptool](https://github.com/espressif/esptool) (`pip install esptool`). From the directory that contains the image, paste the following into **cmd**, substituting your own COM port for `COMx`:

```
python -m esptool --chip esp32 --port COMx --baud 921600 write-flash -z 0x0 marauder_cyd_v1_0x0.bin
```

If `esptool` is already on your PATH, the shorter form is equivalent:

```
esptool --chip esp32 --port COMx --baud 921600 write-flash -z 0x0 marauder_cyd_v1_0x0.bin
```

When the tool prints `Connecting...` and then stalls, enter download mode (hold **BOOT**, tap **RST**, release **BOOT**) and run the command again. Reduce the baud rate to `115200` if the transfer proves unreliable, and note that older esptool releases spell the subcommand `write_flash` with an underscore.

---

## Building the Firmware from Source

Every parameter that affects the output is pinned, so a clone rebuilds the published binary deterministically. A single command performs the entire sequence, installing `arduino-cli`, the ESP32 Arduino core `2.0.17`, and esptool, compiling the sketch against the bundled libraries, and merging the result:

```
bash build.sh
```

The resulting **`marauder_cyd_v1_0x0.bin`** appears in the repository root. On Windows, run the command from Git Bash, and keep the checkout on a short path such as `C:\cyd` to stay clear of the 260 character path limit.

The build is fixed to the following configuration:

| Setting | Value |
|---|---|
| Core | `esp32:esp32@2.0.17` |
| FQBN | `esp32:esp32:esp32:PartitionScheme=min_spiffs,FlashSize=4M,PSRAM=disabled,CPUFreq=240` |
| Linker flag | `-Wl,--allow-multiple-definition` (required, because this codebase defines functions and globals in headers included by many translation units) |
| Merge map | `0x1000` bootloader, `0x8000` partitions, `0xe000` boot_app0, `0x10000` application, combined into one image at `0x0` |

The `min_spiffs` partition scheme is deliberate: it provides the two application slots that the Wi-Fi over-the-air path depends on.

---

## GPS Add-on PCB

The `GPS-NEO_F10N-00B_Files/` directory holds the complete design for the custom GPS daughterboard built around the u-blox **NEO-F10N** receiver, including the KiCad schematic and layout along with the Gerber and drill files needed for independent fabrication. The board carries a u.FL antenna connector through a bias-tee network, a battery-backup supply for the receiver, a regulated feed, and a straightforward UART interface that mates with the display board on IO35 and IO17. Its I²C pass-through is kept clear of the fuel-gauge bus on IO22 and IO27 so the two peripherals coexist cleanly.

---

## Enclosures

Three printable enclosure variants have been designed for the assembly. They are identical in every respect except for how each accommodates the GPS antenna, so the appropriate shell can be chosen to match the receiver configuration in use:

| Variant | GPS antenna provision |
|---|---|
| Passive antenna | A recessed slot sized for a passive ceramic patch antenna, suited to a standard fix with the receiver supplying no external amplification |
| Active antenna | A slot shaped for an active (amplified) antenna, for improved acquisition and sensitivity in weaker-signal environments |
| RTK antenna | A dedicated aperture for an external RTK antenna, enabling a survey-grade antenna to be mounted for high-precision positioning |

## Wi-Fi Over-the-Air Updates

The firmware can update itself without a cable. Place a credential file for one of your networks on the SD card under `/wifi_creds/`, naming the file after the SSID and storing the password as its contents, then open **Device > Update Firmware > WiFi Update**. The device joins the strongest known network, queries the latest release of this repository over HTTPS, downloads the attached image, flashes the spare application slot, and reboots. A merged image is handled automatically: the updater skips the leading `0x10000` so that only the application region is written.

---

## Author and License

Created by **dorukerel3**.

Released under the **MIT License**, see [`LICENSE`](LICENSE).

```
Copyright (c) 2020 Just Call Me Koko
Copyright (c) 2026 dorukerel3
```

Firmware is built upon [ESP32 Marauder](https://github.com/justcallmekoko/ESP32Marauder) by justcallmekoko. The bundled libraries remain under their respective permissive licenses.
