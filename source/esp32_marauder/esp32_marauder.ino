/* FLASH SETTINGS
Board: LOLIN D32
Flash Frequency: 80MHz
Partition Scheme: Minimal SPIFFS
https://www.online-utility.org/image/convert/to/XBM
*/

#include <bb_captouch.h>
#include "configs.h"
#include "TouchDrvGT911.hpp"

TouchDrvGT911 touch;

#if defined(CYD_24CAP) || defined(CYD_22CAP)
BBCapTouch bbct;
const char *szNames[] = {"Unknown", "FT6x36", "GT911", "CST820"};
#endif

#ifndef HAS_SCREEN
  #define MenuFunctions_h
  #define Display_h
#endif

#include <WiFi.h>
#include "EvilPortal.h"
#include <Wire.h>
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include <Arduino.h>

#ifdef HAS_GPS
  #include "GpsInterface.h"
#endif

#include "Assets.h"
#include "WiFiScan.h"
#ifdef HAS_SD
  #include "SDInterface.h"
  #include "FileServerHTTP.h"
#endif
#include "Buffer.h"

#ifdef MARAUDER_FLIPPER
  #include "flipperLED.h"
#elif defined(MARAUDER_V4)
  #include "flipperLED.h"
#elif defined(XIAO_ESP32_S3)
  #include "xiaoLED.h"
#elif defined(MARAUDER_M5STICKC)
  #include "stickcLED.h"
#elif defined(HAS_NEOPIXEL_LED)
  #include "LedInterface.h"
#endif

#include "settings.h"
#include "CommandLine.h"
#include "lang_var.h"

#ifdef HAS_BATTERY
  #include "BatteryInterface.h"
#endif

#ifdef HAS_SCREEN
  #include "Display.h"
  #include "MenuFunctions.h"
#endif

#ifdef HAS_BUTTONS
  #include "Switches.h"
  
  #if (U_BTN >= 0)
    Switches u_btn = Switches(U_BTN, 1000, U_PULL);
  #endif
  #if (D_BTN >= 0)
    Switches d_btn = Switches(D_BTN, 1000, D_PULL);
  #endif
  #if (L_BTN >= 0)
    Switches l_btn = Switches(L_BTN, 1000, L_PULL);
  #endif
  #if (R_BTN >= 0)
    Switches r_btn = Switches(R_BTN, 1000, R_PULL);
  #endif
  #if (C_BTN >= 0)
    Switches c_btn = Switches(C_BTN, 1000, C_PULL);
  #endif
#endif

WiFiScan wifi_scan_obj;
EvilPortal evil_portal_obj;
Buffer buffer_obj;
Settings settings_obj;
CommandLine cli_obj;

#ifdef HAS_GPS
  GpsInterface gps_obj;
#endif

#ifdef HAS_BATTERY
  BatteryInterface battery_obj;
#endif

#ifdef HAS_SCREEN
  Display display_obj;
  MenuFunctions menu_function_obj;
#endif

#ifdef HAS_SD
  SDInterface sd_obj;
#endif

#ifdef MARAUDER_M5STICKC
  AXP192 axp192_obj;
#endif

#ifdef MARAUDER_FLIPPER
  flipperLED flipper_led;
#elif defined(MARAUDER_V4)
  flipperLED flipper_led;
#elif defined(XIAO_ESP32_S3)
  xiaoLED xiao_led;
#elif defined(MARAUDER_M5STICKC)
  stickcLED stickc_led;
#elif defined(HAS_NEOPIXEL_LED)
  LedInterface led_obj;
#endif

const String PROGMEM version_number = MARAUDER_VERSION;

#ifdef HAS_NEOPIXEL_LED
  Adafruit_NeoPixel strip = Adafruit_NeoPixel(Pixels, PIN, NEO_GRB + NEO_KHZ800); // Keeping your current Pixels define
#endif

uint32_t currentTime = 0;

void backlightOn() {
  #ifdef HAS_SCREEN
    #ifdef MARAUDER_MINI
      digitalWrite(TFT_BL, LOW);
    #endif
    #ifndef MARAUDER_MINI
      digitalWrite(TFT_BL, HIGH);
    #endif
  #endif
}

void backlightOff() {
  #ifdef HAS_SCREEN
    #ifdef MARAUDER_MINI
      digitalWrite(TFT_BL, HIGH);
    #endif
    #ifndef MARAUDER_MINI
      digitalWrite(TFT_BL, LOW);
    #endif
  #endif
}

#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Preferences.h>

// Enlarge the Arduino loop task stack from the 8 KB default to 24 KB. The offline router
// runs synchronously down a deep menu -> map -> routeCompute -> A* call chain; on the
// stock 8 KB stack that overflows and panics (a panic with the heap still intact, which
// is exactly what the route calc was doing). A bigger loop stack removes that failure.
SET_LOOP_TASK_STACK_SIZE(24 * 1024);

void setup()
{
  // Disable the brownout detector. On the CYD, heavy SD access (e.g. streaming the
  // routing graph) spikes current draw and the over-sensitive brownout detector can
  // reset the board mid-operation -- the "bootloop while calculating a route". The
  // 3V3 regulator holds up fine in practice; masking the detector stops the resets.
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  esp_spiram_init();

  #ifdef MARAUDER_M5STICKC
    axp192_obj.begin();
  #endif
  
  #ifdef HAS_SCREEN
    pinMode(TFT_BL, OUTPUT);
  #endif
  
  backlightOff();
  
  #if BATTERY_ANALOG_ON == 1
    pinMode(BATTERY_PIN, OUTPUT);
    pinMode(CHARGING_PIN, INPUT);
  #endif
  
  // Preset SPI CS pins to avoid bus conflicts
  #ifdef HAS_SCREEN
    digitalWrite(TFT_CS, HIGH);
  #endif
  
  #ifdef HAS_SD
    pinMode(SD_CS, OUTPUT);
    delay(10);
    digitalWrite(SD_CS, HIGH);
    delay(10);
  #endif

  Serial.begin(115200);

  while (!Serial)
    delay(10);

  Serial.println("ESP-IDF version is: " + String(esp_get_idf_version()));

  #ifdef HAS_SCREEN
    display_obj.RunSetup();
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  #endif

  backlightOff();

  #ifdef HAS_SCREEN
    // ---------- Splash screen ----------
    display_obj.tft.fillScreen(TFT_BLACK);
    {
      int cx = TFT_WIDTH / 2;
      display_obj.tft.setTextDatum(MC_DATUM);
      display_obj.tft.setFreeFont(&FreeSansBold18pt7b);
      display_obj.tft.setTextColor(TFT_RED, TFT_BLACK);
      display_obj.tft.drawString("MARAUDER", cx, (int)(TFT_HEIGHT * 0.40));
      display_obj.tft.setFreeFont(&FreeSansBold12pt7b);
      display_obj.tft.setTextColor(TFT_RED, TFT_BLACK);
      display_obj.tft.drawString("dorukerel3", cx, (int)(TFT_HEIGHT * 0.56));
      display_obj.tft.setFreeFont(NULL);
      display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
      display_obj.tft.drawString("Tap the screen to enter", cx, TFT_HEIGHT - 16, 1);
      display_obj.tft.setTextDatum(TL_DATUM);
    }
  #endif

  backlightOn();

  // If a WiFi OTA was requested from the menu, do it NOW -- before the heavy firmware init below
  // (wifi_scan/menu setup) eats the heap. Only the display is up, so TLS to GitHub has the large
  // contiguous block its handshake needs (this is why the in-UI update failed with "-1 ... b29k").
  {
    Preferences _otap; bool _pend = false;
    if (_otap.begin("marauder", false)) { _pend = _otap.getBool("otaPend", false); if (_pend) _otap.putBool("otaPend", false); _otap.end(); }
    if (_pend) {
      sd_obj.initSD();      // need the SD card for /wifi_creds
      runWifiUpdate();      // reboots on success; returns here on failure and boot continues
    }
  }

  #ifdef HAS_SCREEN
    // ---------- Wait for a real screen tap ----------
    // Idle pressure on this XPT2046 panel reads <= ~250; a real finger press reads > 2000.
    // Threshold 1000 sits cleanly between, so getTouch() is true ONLY for an actual touch.
    // No debounce/baseline logic is needed, and it never auto-skips.
    {
      uint16_t tx = 0, ty = 0;
      pinMode(0, INPUT_PULLUP);                               // BOOT button (GPIO0)
      delay(150);                                             // let the panel settle after boot
      // Enter on a screen tap OR a BOOT-button press. EVERY wait here is bounded,
      // so a touch panel that reads stuck-pressed after a warm reboot can never
      // trap the splash (that was the "stuck on tap-to-enter" bootloop).
      while (true) {
        if (display_obj.tft.getTouch(&tx, &ty, 1000)) break;
        if (digitalRead(0) == LOW) break;
        delay(20);
      }
      // Wait for release, but give up after 1.2 s so a stuck reading still enters.
      uint32_t _rel = millis();
      while ((display_obj.tft.getTouch(&tx, &ty, 1000) || digitalRead(0) == LOW) &&
             (millis() - _rel < 1200))
        delay(10);
      display_obj.tft.fillScreen(TFT_BLACK);
    }
  #endif

  #if defined(CYD_32CAP) || defined(CYD_35CAP)
      #define GT911_SLAVE_ADDRESS_H 0x5D
      #define GT911_SLAVE_ADDRESS_L 0x14
      pinMode(TOUCH_INT, INPUT);
      Wire.begin(TOUCH_SDA, TOUCH_SCL);

      touch.setPins(-1, TOUCH_INT);
      bool touchInitialized = false;
      if (touch.begin(Wire, GT911_SLAVE_ADDRESS_H)) {
          touchInitialized = true;
      } 
      else if (touch.begin(Wire, GT911_SLAVE_ADDRESS_L)) {
          touchInitialized = true;
      }

      if (touchInitialized) {
          Serial.println("Init GT911 Sensor success!");
          // Set touch max xy
          touch.setMaxCoordinates(SCREEN_WIDTH, SCREEN_HEIGHT);
          // Set swap xy
          touch.setSwapXY(false);
          // Set mirror xy
          touch.setMirrorXY(false, false);
      } else {
          Serial.println("Failed to find GT911 at both addresses =[");
          // Could add retry logic here instead of looping forever
      }
  #elif defined(CYD_24CAP)
    bbct.init(TOUCH_SDA, TOUCH_SCL, TOUCH_RST, TOUCH_INT);
    int iType = bbct.sensorType();
    Serial.printf("Sensor type = %s\n", szNames[iType]);
  #elif defined(CYD_22CAP)
    // Attempt CST820 initialization with retry logic
    const int maxRetries = 3;
    int retryCount = 0;
    int iType = -1;
    
    while (retryCount < maxRetries && iType == -1) {
          bbct.init(TOUCH_SDA, TOUCH_SCL, -1, -1);
          iType = bbct.sensorType();
          
          if (iType != -1) {
              Serial.printf("Sensor type = %s (Initialized after %d attempts)\n", 
                          szNames[iType], retryCount + 1);
          } else {
              retryCount++;
              if (retryCount < maxRetries) {
                  Serial.printf("CST820 init failed, attempt %d of %d\n", 
                              retryCount, maxRetries);
                  delay(100); // Short delay between retries
              }
          }
      }
      
      if (iType == -1) {
          Serial.println("Failed to initialize CST820 after all attempts =[");
      }
  #endif

  #ifdef HAS_SCREEN
    // Stealth mode check
    #ifdef HAS_BUTTONS
      if (c_btn.justPressed()) {
        display_obj.headless_mode = true;
        backlightOff();
        Serial.println("Headless Mode enabled");
      }
    #endif

    display_obj.tft.setTextColor(TFT_GREEN, TFT_BLACK);
    display_obj.tft.drawCentreString("Initializing...", TFT_WIDTH / 2, TFT_HEIGHT * 0.82, 1);
  #endif

  settings_obj.begin();
  settings_obj.applyNvsOverrides();   // overlay reliably-saved bool settings from NVS

  wifi_scan_obj.RunSetup();

  buffer_obj = Buffer();

  #if defined(HAS_SD)
    if (sd_obj.initSD()) {
      // No startup text here as per your current code
    } else {
      Serial.println(F("SD Card NOT Supported"));
    }
  #endif

  evil_portal_obj.setup();

  #ifdef HAS_BATTERY
    battery_obj.RunSetup();
  #endif

  #ifdef HAS_BATTERY
    battery_obj.battery_level = battery_obj.getBatteryLevel();
  #endif

  // LED setup
  #ifdef MARAUDER_FLIPPER
    flipper_led.RunSetup();
  #elif defined(MARAUDER_V4)
    flipper_led.RunSetup();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.RunSetup();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.RunSetup();
  #elif defined(HAS_NEOPIXEL_LED)
    led_obj.RunSetup();
  #endif

  #ifdef HAS_GPS
    gps_obj.begin();
  #endif

  #ifdef HAS_SCREEN
    display_obj.tft.setTextColor(TFT_WHITE, TFT_BLACK);
  #endif

  #ifdef HAS_SCREEN
    menu_function_obj.RunSetup();
  #endif

  wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
  
  Serial.println(F("CLI Ready"));
  cli_obj.RunSetup();
}

void loop()
{
  currentTime = millis();
  bool mini = false;

  #ifdef SCREEN_BUFFER
    #if !defined(HAS_ILI9341) && !defined(HAS_ST7789) && !defined(HAS_ST7796)
      mini = true;
    #endif
  #endif

  // Touch disable toggle for all touch-enabled devices
  #if defined(HAS_ILI9341) || defined(HAS_ST7796) || defined(HAS_ST7789)
    #ifdef HAS_BUTTONS
      if (c_btn.isHeld()) {
        menu_function_obj.disable_touch = !menu_function_obj.disable_touch;
        menu_function_obj.updateStatusBar();
        while (!c_btn.justReleased())
          delay(1);
      }
    #endif
  #endif

  cli_obj.main(currentTime);
  #ifdef HAS_SCREEN
    display_obj.main(wifi_scan_obj.currentScanMode);
  #endif
  wifi_scan_obj.main(currentTime);

  #ifdef HAS_GPS
    gps_obj.main();
  #endif
  
  #if defined(HAS_SD)
    sd_obj.main();
  #endif

  buffer_obj.save();

  #ifdef HAS_BATTERY
    battery_obj.main(currentTime);
  #endif
  settings_obj.main(currentTime);

  if (((wifi_scan_obj.currentScanMode != WIFI_PACKET_MONITOR) && (wifi_scan_obj.currentScanMode != WIFI_SCAN_EAPOL)) ||
      (mini)) {
    #ifdef HAS_SCREEN
      menu_function_obj.main(currentTime);
    #endif
  }
  #ifdef MARAUDER_FLIPPER
    flipper_led.main();
  #elif defined(MARAUDER_V4)
    flipper_led.main();
  #elif defined(XIAO_ESP32_S3)
    xiao_led.main();
  #elif defined(MARAUDER_M5STICKC)
    stickc_led.main();
  #elif defined(HAS_NEOPIXEL_LED)
    led_obj.main(currentTime);
  #endif

  #ifdef HAS_SCREEN
    delay(1);
  #else
    delay(50);
  #endif
}