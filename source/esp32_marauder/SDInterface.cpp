#include "SDInterface.h"
#include "lang_var.h"
#include <WiFi.h>
#include "soc/rtc_wdt.h"


bool SDInterface::initSD() {
  #ifdef HAS_SD
    String display_string = "";

    #ifdef KIT
      pinMode(SD_DET, INPUT);
      if (digitalRead(SD_DET) == LOW) {
        Serial.println(F("SD Card Detect Pin Detected"));
      }
      else {
        Serial.println(F("SD Card Detect Pin Not Detected"));
        this->supported = false;
        return false;
      }
    #endif

    pinMode(SD_CS, OUTPUT);

    delay(10);
    #if defined(MARAUDER_M5STICKC)
      /* Set up SPI SD Card using external pin header
      StickCPlus Header - SPI SD Card Reader
                  3v3   -   3v3
                  GND   -   GND
                   G0   -   CLK
              G36/G25   -   MISO
                  G26   -   MOSI
                        -   CS (jumper to SD Card GND Pin)
      */
      enum { SPI_SCK = 0, SPI_MISO = 36, SPI_MOSI = 26 };
      this->spiExt = new SPIClass();
      this->spiExt->begin(SPI_SCK, SPI_MISO, SPI_MOSI, SD_CS);
      if (!SD.begin(SD_CS, *(this->spiExt))) {
    #else
      if (!SD.begin(SD_CS)) {
    #endif
      Serial.println(F("Failed to mount SD Card"));
      this->supported = false;
      return false;
    }
    else {
      this->supported = true;
      this->cardType = SD.cardType();
      //if (cardType == CARD_MMC)
      //  Serial.println(F("SD: MMC Mounted"));
      //else if(cardType == CARD_SD)
      //    Serial.println(F("SD: SDSC Mounted"));
      //else if(cardType == CARD_SDHC)
      //    Serial.println(F("SD: SDHC Mounted"));
      //else
      //    Serial.println(F("SD: UNKNOWN Card Mounted"));

      this->cardSizeMB = SD.cardSize() / (1024 * 1024);
    
      //Serial.printf("SD Card Size: %lluMB\n", this->cardSizeMB);

      if (this->supported) {
        const int NUM_DIGITS = log10(this->cardSizeMB) + 1;

        char sz[NUM_DIGITS + 1];

        sz[NUM_DIGITS] =  0;
        for ( size_t i = NUM_DIGITS; i--; this->cardSizeMB /= 10)
        {
            sz[i] = '0' + (this->cardSizeMB % 10);
            display_string.concat((String)sz[i]);
        }
  
        this->card_sz = sz;
      }

      if (!SD.exists("/SCRIPTS")) {
        Serial.println("/SCRIPTS does not exist. Creating...");

        SD.mkdir("/SCRIPTS");
        Serial.println("/SCRIPTS created");
      }

      this->sd_files = new LinkedList<String>();

      this->sd_files->add("Back");
    
      return true;
  }

  #else
    Serial.println("SD support disabled, skipping init");
    return false;
  #endif
}

File SDInterface::getFile(String path) {
  if (this->supported) {
    File file = SD.open(path, FILE_READ);

    //if (file)
    return file;
  }
}

bool SDInterface::removeFile(String file_path) {
  if (SD.remove(file_path))
    return true;
  else
    return false;
}

void SDInterface::listDirToLinkedList(LinkedList<String>* file_names, String str_dir, String ext) {
  if (this->supported) {
    File dir = SD.open(str_dir);
    while (true)
    {
      File entry = dir.openNextFile();
      if (!entry)
      {
        break;
      }

      if (entry.isDirectory())
        continue;

      String file_name = entry.name();
      if (ext != "") {
        if (file_name.endsWith(ext)) {
          file_names->add(file_name);
        }
      }
      else
        file_names->add(file_name);
    }
  }
}

void SDInterface::listDir(String str_dir){
  if (this->supported) {
    File dir = SD.open(str_dir);
    while (true)
    {
      File entry = dir.openNextFile();
      if (! entry)
      {
        break;
      }
      //for (uint8_t i = 0; i < numTabs; i++)
      //{
      //  Serial.print('\t');
      //}
      Serial.print(entry.name());
      Serial.print("\t");
      Serial.println(entry.size());
      entry.close();
    }
  }
}

void SDInterface::runUpdate() {
  #ifdef HAS_SCREEN
    display_obj.tft.setTextWrap(false);
    display_obj.tft.setFreeFont(NULL);
    display_obj.tft.setCursor(0, TFT_HEIGHT / 3);
    display_obj.tft.setTextSize(1);
    display_obj.tft.setTextColor(TFT_WHITE);
  
    display_obj.tft.println(F("Searching for firmware .bin..."));
  #endif
  // Find the first NON-EMPTY *.bin, looking in /firmware first, then the SD root.
  // Empty (0-byte) files are skipped so a stale leftover can't block a real one.
  File updateBin;
  String fwName = "";
  String fwPath = "";                          // full SD path, for erasing after flash
  const char* searchDirs[2] = {"/firmware", "/"};
  for (int d = 0; d < 2 && !updateBin; d++) {
    File dir = SD.open(searchDirs[d]);
    if (dir && dir.isDirectory()) {
      for (File e = dir.openNextFile(); e; e = dir.openNextFile()) {
        if (!e.isDirectory() && e.size() > 0) {
          String n = String(e.name());
          int slash = n.lastIndexOf('/');
          if (slash >= 0) n = n.substring(slash + 1);
          String low = n; low.toLowerCase();
          if (low.endsWith(".bin")) {
            updateBin = e;        // keep open for flashing
            fwName = n;
            fwPath = (String(searchDirs[d]) == "/") ? ("/" + n) : (String(searchDirs[d]) + "/" + n);
            break;
          }
        }
        e.close();
      }
      dir.close();
    }
  }

  // Legacy fallback: /update.bin at the root (only if non-empty).
  if (!updateBin) {
    File legacy = SD.open("/update.bin");
    if (legacy && !legacy.isDirectory() && legacy.size() > 0) { updateBin = legacy; fwName = "update.bin"; fwPath = "/update.bin"; }
    else if (legacy) legacy.close();
  }

  if (updateBin && !updateBin.isDirectory()) {
    size_t updateSize = updateBin.size();

    // The OTA writer expects an APP-ONLY image. A full "merged" flash image
    // (bootloader @0x1000, partition table @0x8000, app @0x10000 - what gets
    // flashed to offset 0x0) would write the wrong bytes. Detect a merged image
    // by the partition-table magic (0xAA 0x50) at 0x8000 and, if present, flash
    // only the app slice starting at 0x10000.
    size_t appOffset = 0;
    if (updateSize > 0x10000) {
      uint8_t sig[2] = {0, 0};
      updateBin.seek(0x8000);
      updateBin.read(sig, 2);
      updateBin.seek(0);
      if (sig[0] == 0xAA && sig[1] == 0x50) {
        appOffset = 0x10000;
        Serial.println("Merged 0x0 image detected - flashing app slice from 0x10000");
        #ifdef HAS_SCREEN
          display_obj.tft.println("Full image: using app @0x10000");
        #endif
      }
    }
    size_t appSize = updateSize - appOffset;

    #ifdef HAS_SCREEN
      display_obj.tft.println("Found: " + fwName);
    #endif
    Serial.println("Found firmware: " + fwName + " (" + String(updateSize) + " bytes)");

    if (appSize > 0) {
      #ifdef HAS_SCREEN
        display_obj.tft.println(F(text_table2[1]));
      #endif
      Serial.println(F("Starting update over SD. Please wait..."));
      updateBin.seek(appOffset);               // stream the app partition only
      bool ok = this->performUpdate(updateBin, appSize);
      updateBin.close();                        // close before removing

      if (ok) {
        // Erase the file we just flashed so it isn't applied again next time.
        if (fwPath.length() && SD.remove(fwPath.c_str())) {
          Serial.println("Erased flashed file: " + fwPath);
          #ifdef HAS_SCREEN
            display_obj.tft.println("Erased " + fwName);
          #endif
        }
        #ifdef HAS_SCREEN
          display_obj.tft.println(F(text_table2[3]));
        #endif
        Serial.println(F("rebooting..."));
        delay(800);
        // Use the SAME full, clean reset as the Reboot button so the warm boot
        // after an OTA lands in the firmware instead of bootlooping. A plain
        // ESP.restart() (CPU-only reset) left peripherals in a bad state here.
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        btStop();
        pinMode(12, OUTPUT);
        digitalWrite(12, LOW);          // flash-voltage strap low
        delay(10);
        rtc_wdt_protect_off();
        rtc_wdt_set_length_of_reset_signal(RTC_WDT_SYS_RESET_SIG, RTC_WDT_LENGTH_3_2us);
        rtc_wdt_set_stage(RTC_WDT_STAGE0, RTC_WDT_STAGE_ACTION_RESET_SYSTEM);
        rtc_wdt_set_time(RTC_WDT_STAGE0, 50);   // fire after 50 ms
        rtc_wdt_enable();
        rtc_wdt_protect_on();
        while (true) { delay(10); }     // wait for the watchdog to reset us
      } else {
        // Keep the file on failure so the user can retry.
        #ifdef HAS_SCREEN
          display_obj.tft.setTextColor(TFT_RED);
          display_obj.tft.println("Update failed - file kept");
          display_obj.tft.setTextColor(TFT_WHITE);
        #endif
        Serial.println(F("Update failed; keeping file for retry"));
        return;
      }
    }
    else {
      #ifdef HAS_SCREEN
        display_obj.tft.setTextColor(TFT_RED);
        display_obj.tft.println(F(text_table2[2]));
      #endif
      Serial.println(F("Error, file is empty"));
      #ifdef HAS_SCREEN
        display_obj.tft.setTextColor(TFT_WHITE);
      #endif
      updateBin.close();
      return;
    }
  }
  else {
    #ifdef HAS_SCREEN
      display_obj.tft.setTextColor(TFT_RED);
      display_obj.tft.println(F("No .bin found on SD"));
      display_obj.tft.println(F("(put it in /firmware or root)"));
    #endif
    Serial.println(F("No non-empty .bin found in /firmware or SD root"));
    #ifdef HAS_SCREEN
      display_obj.tft.setTextColor(TFT_WHITE);
    #endif
  }
}

bool SDInterface::performUpdate(Stream &updateSource, size_t updateSize) {
  bool success = false;
  if (Update.begin(updateSize)) {
    #ifdef HAS_SCREEN
      display_obj.tft.println(text_table2[5] + String(updateSize));
      display_obj.tft.println(F(text_table2[6]));
    #endif
    size_t written = Update.writeStream(updateSource);
    if (written == updateSize) {
      #ifdef HAS_SCREEN
        display_obj.tft.println(text_table2[7] + String(written) + text_table2[10]);
      #endif
      Serial.println("Written : " + String(written) + " successfully");
    }
    else {
      #ifdef HAS_SCREEN
        display_obj.tft.println(text_table2[8] + String(written) + "/" + String(updateSize) + text_table2[9]);
      #endif
      Serial.println("Written only : " + String(written) + "/" + String(updateSize) + ". Retry?");
    }
    if (Update.end()) {
      Serial.println("OTA done!");
      if (Update.isFinished()) {
        #ifdef HAS_SCREEN
          display_obj.tft.println(F(text_table2[11]));
        #endif
        Serial.println(F("Update successfully completed. Rebooting."));
        success = true;
      }
      else {
        #ifdef HAS_SCREEN
          display_obj.tft.setTextColor(TFT_RED);
          display_obj.tft.println(text_table2[12]);
        #endif
        Serial.println("Update not finished? Something went wrong!");
        #ifdef HAS_SCREEN
          display_obj.tft.setTextColor(TFT_WHITE);
        #endif
      }
    }
    else {
      #ifdef HAS_SCREEN
        display_obj.tft.println(text_table2[13] + String(Update.getError()));
      #endif
      Serial.println("Error Occurred. Error #: " + String(Update.getError()));
    }

  }
  else
  {
    #ifdef HAS_SCREEN
      display_obj.tft.println(text_table2[14]);
    #endif
    Serial.println("Not enough space to begin OTA");
  }
  return success;
}

bool SDInterface::checkDetectPin() {
  #ifdef KIT
    if (digitalRead(SD_DET) == LOW)
      return true;
    else
      return false;
  #endif

  return false;
}

void SDInterface::main() {
  if (!this->supported) {
    if (checkDetectPin()) {
      delay(100);
      this->initSD();
    }
  }
}
