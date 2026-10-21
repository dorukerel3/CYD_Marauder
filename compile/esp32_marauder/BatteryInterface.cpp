#include "BatteryInterface.h"
#include "lang_var.h"
BatteryInterface::BatteryInterface() {
  
}

void BatteryInterface::main(uint32_t currentTime) {
  if (currentTime != 0 && this->i2c_supported) { // Only poll if I2C is confirmed
    if (currentTime - initTime >= 3000) {
      this->initTime = millis();
      int8_t new_level = this->getBatteryLevel();
      if (this->battery_level != new_level) {
        this->battery_level = new_level;
        Serial.println("Battery Level: " + (String)this->battery_level);
      }
    }
  }
  #ifdef HAS_BATTERY
  else if (currentTime != 0) {   // no gauge yet: retry detection so a hot-plugged one appears
    if (currentTime - initTime >= 3000) {
      this->initTime = millis();
      this->reinit();
    }
  }
  #endif
}

void BatteryInterface::RunSetup() {
  #ifdef HAS_BATTERY
    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.println("Checking for battery monitors...");

    // Check IP5306
    Wire.beginTransmission(IP5306_ADDR);
    byte error = Wire.endTransmission();
    if (error == 0) {
      Serial.println("Detected IP5306");
      this->has_ip5306 = true;
      this->i2c_supported = true;
    } else {
      Serial.println("No IP5306 found");
      this->has_ip5306 = false;
    }

    // Check MAX1704X family (MAX17048/49 AND MAX17043/44).
    // NOTE: maxlipo.begin() gates on getICversion()==0x001X, which only the
    // MAX17048/49 return; a genuine MAX17043 fails that check even though its
    // SOC register (0x04) is read identically by cellPercent(). So we detect by
    // the 0x36 I2C ACK and call begin() only to set up the bus object, ignoring
    // its return value. Works for both chips; harmless on a relabeled 17048.
    Wire.beginTransmission(MAX17048_ADDR);
    error = Wire.endTransmission();
    if (error == 0) {
      maxlipo.begin(&Wire); // initializes i2c_dev; return ignored (MAX17043 fails version gate)
      Serial.println("Detected MAX1704X @ 0x36 (MAX17048/49 or MAX17043/44)");
      this->has_max17048 = true;
      this->i2c_supported = true;
    } else {
      Serial.println("No MAX1704X found");
      this->has_max17048 = false;
    }

    // Only enable i2c_supported if a device is actually present
    this->i2c_supported = (this->has_ip5306 || this->has_max17048);
    if (!this->i2c_supported) {
      Serial.println("No battery monitors detected; disabling I2C polling");
    }

    this->initTime = millis();
  #else
    this->i2c_supported = false; // Ensure disabled if no HAS_BATTERY
  #endif
}

// Live-probe the MAX1704x fuel gauge (MAX17048/49 or MAX17043/44) at 0x36 and
// set it up if present. Lets the Fuel Gauge screen read a gauge that was plugged
// in after boot, without needing a reboot. Returns true if the gauge is present.
bool BatteryInterface::reinit() {
  #ifdef HAS_BATTERY
    Wire.begin(I2C_SDA, I2C_SCL);
    Wire.beginTransmission(MAX17048_ADDR);
    if (Wire.endTransmission() == 0) {
      this->maxlipo.begin(&Wire);   // return ignored (MAX17043 fails the version gate)
      this->has_max17048 = true;
      this->i2c_supported = true;
      return true;
    }
    this->has_max17048 = false;
  #endif
  return false;
}

float BatteryInterface::getVoltage() {
  #ifdef HAS_BATTERY
    if (this->has_max17048) return this->maxlipo.cellVoltage();
  #endif
  return -1.0f;
}

int8_t BatteryInterface::getBatteryLevel() {
  if (!this->i2c_supported) {
    return -1; // No monitor present
  }

  if (this->has_ip5306) {
    Wire.beginTransmission(IP5306_ADDR);
    Wire.write(0x78);
    if (Wire.endTransmission(false) == 0 && Wire.requestFrom(IP5306_ADDR, 1)) {
      uint8_t val = Wire.read();
      switch (val & 0xF0) {
        case 0xE0: return 25;
        case 0xC0: return 50;
        case 0x80: return 75;
        case 0x00: return 100;
        default: return 0;
      }
    }
    Serial.println("IP5306 read failed");
    this->i2c_supported = false; // Disable on failure
    return -1;
  }

  if (this->has_max17048) {
    float percent = this->maxlipo.cellPercent();
    if (percent >= 100) return 100;
    if (percent <= 0) return 0;
    return (int8_t)percent;
  }

  return -1; // Fallback if no device is active
}