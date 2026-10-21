# CYD_Marauder

Custom **ESP32 Marauder** firmware for the **dual‑USB "Cheap Yellow Display" (CYD2USB / ESP32‑2432S028R)**, plus the **custom NEO‑F10N GPS add‑on PCB** that goes with it.

This is a modified fork with a reworked touch UI, extra hardware support (bidirectional GPS + battery fuel gauge), an offline SD map viewer, a Wi‑Fi file server, Wi‑Fi OTA updates, and a multi‑language menu. It builds into a single image you flash at offset `0x0`.

> ⚠️ **For authorized security testing and education only.** ESP32 Marauder is a Wi‑Fi/Bluetooth analysis and testing tool. Only use it on networks and devices you own or have explicit permission to test. You are responsible for complying with the laws in your jurisdiction.

---

## Repository layout

| Path | What it is |
|---|---|
| `source/` | The Arduino sketch (`source/esp32_marauder/`) **and every library it needs** (`source/libs/`). This is what the build uses. |
| `compile/` | A self‑contained copy of the sketch + libraries (mirror of `source/`), handy for building in one folder. |
| `build.sh` | One‑command build: installs the toolchain, compiles, and merges the flashable `0x0` image. |
| `marauder_cyd_v2_0x0.bin` | Pre‑built merged firmware image (flash at `0x0`). |
| `GPS-NEO_F10N-00B_Files/` | The custom u‑blox **NEO‑F10N** GPS add‑on PCB (KiCad project + Gerbers). |
| `CLOUD_BUILD.md` | Notes for building in a cloud/Linux sandbox and publishing a Release. |

---

## Build it yourself — one command

Clone the repo and run a single command; it installs `arduino-cli` + the **ESP32 Arduino core `2.0.17`** + `esptool`, compiles the sketch against the bundled libraries, and merges the exact same `marauder_cyd_v2_0x0.bin` you can flash:

```bash
bash build.sh
```

Output: **`marauder_cyd_v2_0x0.bin`** in the repo root. (On Windows, run it from Git Bash; everything is pinned — core version, libraries, partition scheme and linker flag — so the image reproduces byte‑for‑byte.)

The build is fixed to:

- **Board / FQBN:** `esp32:esp32:esp32:PartitionScheme=min_spiffs,FlashSize=4M,PSRAM=disabled,CPUFreq=240`
- **Linker flag:** `-Wl,--allow-multiple-definition` (this codebase defines functions/globals in headers included by multiple translation units — required).
- **Merge map:** `0x1000` bootloader, `0x8000` partitions, `0xe000` boot_app0, `0x10000` app → one image at `0x0`.

---

## Flashing (pre‑built binary)

The flashable image is **`marauder_cyd_v2_0x0.bin`** — a merged image (bootloader + partitions + boot_app0 + app) that writes as a single file at offset `0x0`. Plug into the **micro‑USB** port before flashing.

### Option A — Web flasher (no install)

The easiest way: open the browser flasher at **[dorukerel.com](https://dorukerel.com)** in **Chrome or Edge** (they support WebSerial), plug the board into the **micro‑USB** port, click **Connect**, pick the COM port, and flash `marauder_cyd_v2_0x0.bin` at offset `0x0`. No Python or drivers to set up.

If it won't connect, put the board in download mode first: **hold BOOT → tap RST → release BOOT**, then retry.

### Option B — Command line (esptool)

Requires Python + [esptool](https://github.com/espressif/esptool) (`pip install esptool`). Paste this into **cmd** from the folder that contains the `.bin` (replace `COM6` with your port):

```bash
python -m esptool --chip esp32 --port COM6 --baud 921600 write-flash -z 0x0 marauder_cyd_v2_0x0.bin
```

Or, if `esptool` is on your PATH:

```bash
esptool --chip esp32 --port COM6 --baud 921600 write-flash -z 0x0 marauder_cyd_v2_0x0.bin
```

If it prints `Connecting...` and fails, put the board in download mode (**hold BOOT → tap RST → release BOOT**) and run again. Drop the baud to `115200` if it's flaky. On older esptool use `write_flash` (underscore) instead of `write-flash`.

---

## What's in this fork

- **Dual‑USB (CYD2USB) display fixed** — correct colours on the 2‑USB board.
- **Bidirectional GPS** on pins **IO35 (RX)** / **IO17 (TX)**, with runtime‑selectable pins/baud in *Device > GPS Settings*. The **GPS Data** and **NMEA Stream** screens show the live link **baud** and measured **fix rate (Hz)**.
- **MAX17043/MAX17048 battery fuel gauge** over I²C on **IO22 (SDA) / IO27 (SCL)** (auto‑detected); battery % + icon in the status bar.
- **Offline map viewer** — OpenStreetMap raster tiles on SD, pan/zoom, centre crosshair, live GPS marker + follow, save/recall favourites.
- **Wi‑Fi file server** — joins your network (or self‑hosts an AP) and serves the SD card over HTTP + FTP.
- **Wi‑Fi OTA update** — pulls the latest GitHub Release `.bin` over HTTPS and flashes the spare OTA slot.
- **Paginated touch menus, corner‑back navigation, 7‑language menu UI** (saved to NVS, persists across reboots).

## Hardware

- **Board:** ESP32‑2432S028R "Cheap Yellow Display", **2‑USB variant**. ESP32‑WROOM‑32U (external antenna) — **attach a 2.4 GHz antenna to the u.FL/IPEX connector** before any Wi‑Fi work.
- **GPS PCB:** see `GPS-NEO_F10N-00B_Files/` for the u‑blox NEO‑F10N add‑on board (KiCad sources + Gerbers).

| Signal | CYD pin |
|---|---|
| GPS TX → ESP **RX** | **IO35** |
| ESP **TX** → GPS RX | **IO17** |
| Fuel gauge **SDA / SCL** | **IO22 / IO27** |

Flash for the CYD2USB uses the **micro‑USB** port (the USB‑C port lacks CC resistors).

---

## Credits

- **[ESP32 Marauder](https://github.com/justcallmekoko/ESP32Marauder)** by *justcallmekoko* — the original project (MIT).
- **[ESP32‑Marauder‑Cheap‑Yellow‑Display](https://github.com/Fr4nkFletcher/ESP32-Marauder-Cheap-Yellow-Display)** by *Fr4nkFletcher* — the CYD fork this is based on.
- Bundled libraries (TFT_eSPI, U8g2, Adafruit GFX/BusIO/MAX1704X, NimBLE‑Arduino, ArduinoJson, LVGL, MicroNMEA, PNGdec, and others) remain under their respective permissive licenses.

## License

Released under the **MIT License** — see [`LICENSE`](LICENSE).

```
Copyright (c) 2020 Just Call Me Koko
Copyright (c) 2026 dorukerel3
```
