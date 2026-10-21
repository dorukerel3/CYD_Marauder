#pragma once

#ifndef Settings_h
#define Settings_h

#include "configs.h"

#include "SPIFFS.h"
#include <FS.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#define FORMAT_SPIFFS_IF_FAILED true

#ifdef HAS_SCREEN
  #include "Display.h"

  extern Display display_obj;
#endif

class Settings {

  private:
    String json_settings_string;

  public:
    bool begin();

    template <typename T>
    T loadSetting(String name);

    template <typename T>
    T saveSetting(String key, bool value);

    bool toggleSetting(String key);
    String getSettingType(String key);
    String setting_index_to_name(int i);
    int getNumberSettings();

    //template<>
    //int loadSetting<int>(String key);
    
    //template<>
    //String loadSetting<String>(String key);
    
    //template<>
    //bool loadSetting<bool>(String key);
    
    //template<>
    //uint8_t loadSetting<uint8_t>(String key);

    String getSettingsString();
    bool createDefaultSettings(fs::FS &fs);
    void applyNvsOverrides();   // overlay saved bool settings from NVS (reliable persistence)
    void printJsonSettings(String json_string);
    void main(uint32_t currentTime);
};

// Explicit specialization declarations so EVERY translation unit binds to the real
// implementations in settings.cpp (not the empty primary template, which would
// always return false and make all settings read as "disabled").
template<> bool Settings::loadSetting<bool>(String name);
template<> int Settings::loadSetting<int>(String name);
template<> String Settings::loadSetting<String>(String name);
template<> uint8_t Settings::loadSetting<uint8_t>(String name);
template<> bool Settings::saveSetting<bool>(String key, bool value);

#endif
