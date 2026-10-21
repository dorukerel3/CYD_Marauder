#include "settings.h"

String Settings::getSettingsString() {
  return this->json_settings_string;
}

bool Settings::begin() {
  if(!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED)){
    Serial.println("Settings SPIFFS Mount Failed");
    return false;
  }

  File settingsFile;

  //SPIFFS.remove("/settings.json"); // NEED TO REMOVE THIS LINE

  if (SPIFFS.exists("/settings.json")) {
    settingsFile = SPIFFS.open("/settings.json", FILE_READ);
    
    if (!settingsFile) {
      settingsFile.close();
      Serial.println(F("Could not find settings file"));
      if (this->createDefaultSettings(SPIFFS))
        return true;
      else
        return false;    
    }
  }
  else {
    Serial.println("Settings file does not exist");
    if (this->createDefaultSettings(SPIFFS))
      return true;
    else
      return false;
  }

  String json_string;
  DynamicJsonDocument jsonBuffer(1024);
  DeserializationError error = deserializeJson(jsonBuffer, settingsFile);
  serializeJson(jsonBuffer, json_string);
  //Serial.println("Settings: " + (String)json_string + "\n");
  //this->printJsonSettings(json_string);

  this->json_settings_string = json_string;
  
  return true;
}

template <typename T>
T Settings::loadSetting(String key) {}

// Get type int settings
template<>
int Settings::loadSetting<int>(String key) {
  DynamicJsonDocument json(1024); // ArduinoJson v6

  if (deserializeJson(json, this->json_settings_string)) {
    Serial.println("\nCould not parse json");
  }

  for (int i = 0; i < json["Settings"].size(); i++) {
    if (json["Settings"][i]["name"].as<String>() == key)
      return json["Settings"][i]["value"];
  }

  return 0;
}

// Get type string settings
template<>
String Settings::loadSetting<String>(String key) {
  //return this->json_settings_string;
  
  DynamicJsonDocument json(1024); // ArduinoJson v6

  if (deserializeJson(json, this->json_settings_string)) {
    Serial.println("\nCould not parse json");
  }

  for (int i = 0; i < json["Settings"].size(); i++) {
    if (json["Settings"][i]["name"].as<String>() == key)
      return json["Settings"][i]["value"];
  }

  return "";
}

// Get type bool settings.
// Reads the in-RAM JSON copy (fast — this is called per-packet during capture,
// so it must NOT hit NVS/flash every call). The in-RAM copy is kept in sync
// with NVS at boot (applyNvsOverrides) and on every saveSetting. Using
// .as<bool>() rather than raw variant truthiness avoids any ambiguity from a
// value persisted in an unexpected form.
template<>
bool Settings::loadSetting<bool>(String key) {
  DynamicJsonDocument json(1024); // ArduinoJson v6

  if (deserializeJson(json, this->json_settings_string)) {
    Serial.println("\nCould not parse json");
  }

  for (int i = 0; i < json["Settings"].size(); i++) {
    if (json["Settings"][i]["name"].as<String>() == key)
      return json["Settings"][i]["value"].as<bool>();
  }

  return false;
}

//Get type uint8_t settings
template<>
uint8_t Settings::loadSetting<uint8_t>(String key) {
  DynamicJsonDocument json(1024); // ArduinoJson v6

  if (deserializeJson(json, this->json_settings_string)) {
    Serial.println("\nCould not parse json");
  }

  for (int i = 0; i < json["Settings"].size(); i++) {
    if (json["Settings"][i]["name"].as<String>() == key)
      return json["Settings"][i]["value"];
  }

  return 0;
}

template <typename T>
T Settings::saveSetting(String key, bool value) {}

template<>
bool Settings::saveSetting<bool>(String key, bool value) {
  DynamicJsonDocument json(1024); // ArduinoJson v6

  if (deserializeJson(json, this->json_settings_string)) {
    Serial.println("\nCould not parse json");
  }

  String settings_string;

  for (int i = 0; i < json["Settings"].size(); i++) {
    if (json["Settings"][i]["name"].as<String>() == key) {
      json["Settings"][i]["value"] = value;

      Serial.println("Saving setting...");

      // Update the in-RAM authoritative copy FIRST, so the setting takes effect
      // immediately (and the UI shows it) even if the flash write can't be made.
      if (serializeJson(json, settings_string) == 0)
        Serial.println(F("Failed to write to string"));
      this->json_settings_string = settings_string;

      // Persist to NVS (reliable on this board even when SPIFFS writes fail).
      {
        Preferences _p;
        if (_p.begin("mset", false)) { _p.putBool(key.c_str(), value); _p.end(); }
      }

      // Best-effort persist to flash (SPIFFS). If this fails the change still
      // applies for this session.
      File settingsFile = SPIFFS.open("/settings.json", FILE_WRITE);
      if (settingsFile) {
        if (serializeJson(json, settingsFile) == 0)
          Serial.println(F("Failed to write settings to flash"));
        settingsFile.close();
      } else {
        Serial.println(F("Could not open settings file; change kept in RAM only"));
      }

      this->printJsonSettings(this->json_settings_string);
      return true;
    }
  }
  return false;
}

bool Settings::toggleSetting(String key) {
  // Read the current *effective* value (NVS-authoritative) and flip it.
  // This avoids depending on JSON truthiness, which could be always-true for
  // a value persisted in an unexpected form and would wedge the toggle.
  bool current = this->loadSetting<bool>(key);
  bool next = !current;
  this->saveSetting<bool>(key, next);
  Serial.print("Toggling "); Serial.print(key);
  Serial.println(next ? " -> true" : " -> false");
  return next;
}

String Settings::setting_index_to_name(int i) {
  DynamicJsonDocument json(1024); // ArduinoJson v6

  if (deserializeJson(json, this->json_settings_string)) {
    Serial.println("\nCould not parse json");
  }

  return json["Settings"][i]["name"];
}

int Settings::getNumberSettings() {
  DynamicJsonDocument json(1024); // ArduinoJson v6

  if (deserializeJson(json, this->json_settings_string)) {
    Serial.println("\nCould not parse json");
  }

  return json["Settings"].size();
}

String Settings::getSettingType(String key) {
  DynamicJsonDocument json(1024); // ArduinoJson v6

  if (deserializeJson(json, this->json_settings_string)) {
    Serial.println("\nCould not parse json");
  }
  
  for (int i = 0; i < json["Settings"].size(); i++) {
    if (json["Settings"][i]["name"].as<String>() == key)
      return json["Settings"][i]["type"];
  }
}

void Settings::printJsonSettings(String json_string) {
  DynamicJsonDocument json(1024); // ArduinoJson v6

  if (deserializeJson(json, json_string)) {
    Serial.println("\nCould not parse json");
  }
  
  Serial.println("Settings\n----------------------------------------------");
  for (int i = 0; i < json["Settings"].size(); i++) {
    Serial.println("Name: " + json["Settings"][i]["name"].as<String>());
    Serial.println("Type: " + json["Settings"][i]["type"].as<String>());
    Serial.println("Value: " + json["Settings"][i]["value"].as<String>());
    Serial.println("----------------------------------------------");
  }
}

bool Settings::createDefaultSettings(fs::FS &fs) {
  Serial.println(F("Creating default settings file: settings.json"));
  
  File settingsFile = fs.open("/settings.json", FILE_WRITE);

  if (!settingsFile) {
    Serial.println(F("Failed to create settings file"));
    return false;
  }

  DynamicJsonDocument jsonBuffer(1024);
  String settings_string;

  //jsonBuffer["Settings"][0]["name"] = "Channel";
  //jsonBuffer["Settings"][0]["type"] = "uint8_t";
  //jsonBuffer["Settings"][0]["value"] = 11;
  //jsonBuffer["Settings"][0]["range"]["min"] = 1;
  //jsonBuffer["Settings"][0]["range"]["max"] = 14;

  //jsonBuffer["Settings"][1]["name"] = "Channel Hop Delay";
  //jsonBuffer["Settings"][1]["type"] = "int";
  //jsonBuffer["Settings"][1]["value"] = 1;
  //jsonBuffer["Settings"][1]["range"]["min"] = 1;
  //jsonBuffer["Settings"][1]["range"]["max"] = 10;

  jsonBuffer["Settings"][0]["name"] = "ForcePMKID";
  jsonBuffer["Settings"][0]["type"] = "bool";
  jsonBuffer["Settings"][0]["value"] = true;
  jsonBuffer["Settings"][0]["range"]["min"] = false;
  jsonBuffer["Settings"][0]["range"]["max"] = true;

  jsonBuffer["Settings"][1]["name"] = "ForceProbe";
  jsonBuffer["Settings"][1]["type"] = "bool";
  jsonBuffer["Settings"][1]["value"] = true;
  jsonBuffer["Settings"][1]["range"]["min"] = false;
  jsonBuffer["Settings"][1]["range"]["max"] = true;

  jsonBuffer["Settings"][2]["name"] = "SavePCAP";
  jsonBuffer["Settings"][2]["type"] = "bool";
  jsonBuffer["Settings"][2]["value"] = true;
  jsonBuffer["Settings"][2]["range"]["min"] = false;
  jsonBuffer["Settings"][2]["range"]["max"] = true;

  jsonBuffer["Settings"][3]["name"] = "EnableLED";
  jsonBuffer["Settings"][3]["type"] = "bool";
  jsonBuffer["Settings"][3]["value"] = true;
  jsonBuffer["Settings"][3]["range"]["min"] = false;
  jsonBuffer["Settings"][3]["range"]["max"] = true;

  //jsonBuffer.printTo(settingsFile);
  if (serializeJson(jsonBuffer, settingsFile) == 0) {
    Serial.println(F("Failed to write to file"));
  }
  if (serializeJson(jsonBuffer, settings_string) == 0) {
    Serial.println(F("Failed to write to string"));
  }

  // Close the file
  settingsFile.close();

  this->json_settings_string = settings_string;

  this->printJsonSettings(settings_string);

  return true;
}

// Overlay any bool settings saved in NVS onto the in-RAM settings (NVS is the
// reliable store on this board; SPIFFS writes may fail). Call once after begin().
void Settings::applyNvsOverrides() {
  Preferences _p;
  if (!_p.begin("mset", false)) return;   // read-write: we may seed defaults
  DynamicJsonDocument json(1024);
  if (deserializeJson(json, this->json_settings_string)) { _p.end(); return; }
  bool changed = false;
  for (int i = 0; i < json["Settings"].size(); i++) {
    if (json["Settings"][i]["type"].as<String>() != "bool") continue;
    String name = json["Settings"][i]["name"].as<String>();
    bool json_default = json["Settings"][i]["value"].as<bool>();
    bool effective;
    if (_p.isKey(name.c_str())) {
      // NVS wins — it's the reliable persistent store on this board.
      effective = _p.getBool(name.c_str(), json_default);
    } else {
      // First boot with this setting: seed NVS from the JSON default so the
      // stored value and the UI agree from the start.
      effective = json_default;
      _p.putBool(name.c_str(), effective);
    }
    // Force the RAM copy to a real bool matching NVS.
    json["Settings"][i]["value"] = effective;
    changed = true;
  }
  _p.end();
  if (changed) {
    String s;
    serializeJson(json, s);
    this->json_settings_string = s;
  }
}

void Settings::main(uint32_t currentTime) {

}
