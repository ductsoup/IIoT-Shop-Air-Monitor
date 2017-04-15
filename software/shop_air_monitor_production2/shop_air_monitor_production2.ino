//
// Wiring
//
//  USB -> +5V
//  GND -> GND
//  A1  -> voltage divider
//  D6  -> Neopixel Din
//

// FlashStorage
// https://github.com/cmaglie/FlashStorage
#include "FlashAsEEPROM.h"
typedef struct {
  boolean valid;
  float thresh_idle;
  float thresh_low;
  float thresh_high;
  float update_interval;
  float filter_weight;
  float psig_offset;
} FLASH_CONFIG;
FlashStorage(flash_config, FLASH_CONFIG);
FLASH_CONFIG saved_config;

//
// Neopixel setup
//
#define PIN 6
#define LED_COUNT 64 // max 12 for testing without 24VDC PS enabled
#include "neopixel.h"

//
// MODBUS WiFi setup
//
char* access_points[][2] = {
  {"MySSID", "MyPassword"}
};
#include "ModbusTCP.h"
//
// MODBUS register map
//
#define S0_CurrentMillis   40001 // modpoll -m tcp -t 4:float -r 40001 [ip_addr]
#define S0_RSSI            40003
#define S0_MODE            40005
#define S0_THRESH_IDLE     40007 // retained
#define S0_THRESH_LOW      40009 // retained
#define S0_THRESH_HIGH     40011 // retained
#define S1_UPDATE_INTERVAL 40013 // retained
#define S1_FILTER_WEIGHT   40015 // retained
#define S1_PSIG_RAW        40017
#define S1_PSIG_OFFSET     40019 // retained
#define S1_PSIG            40021
#define S2_R               40023
#define S2_G               40025
#define S2_B               40027
#define S0_STORE_CONFIG    40029
unsigned long S1StartTime = millis();

void setup() {
  Serial.begin(9600);

  Serial.print(sizeof(FLASH_CONFIG));

  strip.begin();
  strip.setPixelColor(0, strip.Color(16, 0, 0)); // wake
  strip.show();

  delay(10000); // Give the BA cap time to charge
  strip.setPixelColor(0, strip.Color(0, 0, 16)); // not connected
  strip.show();

  saved_config = flash_config.read();
  if (!saved_config.valid) {
    Serial.println("Invalid configuration");
    saved_config.valid = true;
    saved_config.thresh_idle = 20;
    saved_config.thresh_low = 80;
    saved_config.thresh_high = 120;
    saved_config.update_interval = 500;
    saved_config.filter_weight = 4;
    saved_config.psig_offset = 0;
    setFloat(S0_STORE_CONFIG, 255);
  }
  else
    setFloat(S0_STORE_CONFIG, 0);
  setU16(S0_MODE, 255);
  setFloat(S0_THRESH_IDLE, saved_config.thresh_idle);
  setFloat(S0_THRESH_LOW, saved_config.thresh_low);
  setFloat(S0_THRESH_HIGH, saved_config.thresh_high);
  setFloat(S1_UPDATE_INTERVAL, saved_config.update_interval);
  setFloat(S1_FILTER_WEIGHT, saved_config.filter_weight);
  setFloat(S1_PSIG_OFFSET, saved_config.psig_offset);

  analogReadResolution(12);           // bump the ADC resolution from 10 to 12 bits because we can

  modbus_begin();

  strip.setPixelColor(0, strip.Color(0, 16, 0)); // connected
  strip.show();
}

void loop() {
  uint32_t curMillis = millis();
  static int16_t i, j, k;
  byte r, g, b;
  static byte mode, last_mode;
  float psig, ti, tl, th;

  //
  // Save configuration if this register contains 255, confirm the
  // configuration has been saved by resetting the register to 0
  //
  if (getFloat(S0_STORE_CONFIG) == 255.0) {
    for (i = 0; i < strip.numPixels(); i++)
      strip.setPixelColor(i, strip.Color(0, 16, 16));
    strip.show();
    saved_config.valid = true;
    saved_config.thresh_idle = getFloat(S0_THRESH_IDLE);
    saved_config.thresh_low = getFloat(S0_THRESH_LOW);
    saved_config.thresh_high = getFloat(S0_THRESH_HIGH);
    saved_config.update_interval = getFloat(S1_UPDATE_INTERVAL);
    saved_config.filter_weight = getFloat(S1_FILTER_WEIGHT);
    saved_config.psig_offset = getFloat(S1_PSIG_OFFSET);
    flash_config.write(saved_config);
    setFloat(S0_STORE_CONFIG, 0);
    Serial.println("Wrote configuration to FLASH memory");
  }

  //
  // Update MODBUS registers
  //
  setFloat(S0_CurrentMillis, millis());
  setFloat(S0_RSSI, WiFi.RSSI());

  //
  // The sensor reports 0-10V from 0-200 psig and there is a 3:1 voltage divider to
  // scale 0-10 to 0-3.3. The analog input reports 0-3.3V from 0 to 4096. The divider
  // and the reference cancel out. To read in psig multiply by the sensor's full
  // scale then divide by the ADC steps.
  //
  if (curMillis - S1StartTime > getFloat(S1_UPDATE_INTERVAL)) {
    psig = 200.0 * analogRead(A1) / 4096.0;
    setFloat(S1_PSIG_RAW, psig);
    setFloat(S1_PSIG, max((psig + getFloat(S1_PSIG_OFFSET) + getFloat(S1_PSIG) * (getFloat(S1_FILTER_WEIGHT) - 1.0)) / getFloat(S1_FILTER_WEIGHT), 0));
    S1StartTime = curMillis;
  }

  //
  // Process annunciator actions
  //
  mode = byte(getFloat(S0_MODE));
  switch (mode) {
    //
    //    0 to  20 Low Blue
    //   20 to  80 Flash Red
    //   80 to 120 Low Green
    //    over 120 Flash Red
    //
    case 0: // automatic
      if (mode != last_mode) j = 0;
      ti = getFloat(S0_THRESH_IDLE);
      tl = getFloat(S0_THRESH_LOW);
      th = getFloat(S0_THRESH_HIGH);
      psig = getFloat(S1_PSIG);
      // idle
      if (getFloat(S1_PSIG) < ti) {
        for (i = 0; i < strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(0, 0, 4));
          modbus_run();
        }
      }
      // pressure is too low
      else if (getFloat(S1_PSIG) >= ti && getFloat(S1_PSIG) < tl) {
        --j = j > 0 ? j : 255;
        for (i = 0; i < strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(j, 0, 0));
          modbus_run();
        }
      }
      // pressure is ok
      else if (getFloat(S1_PSIG) >= tl && getFloat(S1_PSIG) < th)  {
        --j = j > 0 ? j : 255;
        for (i = 0; i < strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(0, 16, 0));
          modbus_run();
        }
      }
      // pressure is too high
      else {
        --j = j > 0 ? j : 255;
        for (i = 0; i < strip.numPixels(); i++) {
          strip.setPixelColor(i, strip.Color(j, j, 0));
          modbus_run();
        }
      }
      strip.show();
      break;

    case 1: // all off
      for (i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(0, 0, 0));
        modbus_run();
      }
      strip.show();
      break;

    case 2: // all on at a preset color
      r = constrain(getFloat(S2_R), 0, 255);
      g = constrain(getFloat(S2_G), 0, 255);
      b = constrain(getFloat(S2_B), 0, 255);
      for (i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(r, g, b));
        modbus_run();
      }
      strip.show();
      break;

    case 3: // pulse
      if (mode != last_mode) j = 255;
      r = constrain(getFloat(S2_R), 0, 1);
      g = constrain(getFloat(S2_G), 0, 1);
      b = constrain(getFloat(S2_B), 0, 1);
      if (!(r || g || b)) {
        r = 1;
        g = b = 0;
      }
      --j = j > 0 ? j : 255;
      for (i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(j * r, j * g, j * b));
        modbus_run();
      }
      strip.show();
      break;

    case 4: // rainbow attention getter
      if (mode != last_mode) j = 0;
      ++j = j > 255 ? 0 : j;
      for (i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, Wheel(j));
        modbus_run();
      }
      strip.show();
      //Serial.println(j);
      break;

    default:
      setFloat(S0_MODE, 0);
      modbus_run();
      break;
  }
  last_mode = mode;
}

