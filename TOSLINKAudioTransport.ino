#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "AudioEngine.h"
#include "DisplayUI.h"
#include "LibraryCommon.h"

#define FT6336U_ADDR 0x38
#define RESTART_TEENSY() *(volatile uint32_t *)0xE000ED0C = 0x5FA0004

void setup() {
  // =========================================================================
  // 🚀 PHASE 0: PARASITIC POWER PURGE (The Hard Drain)
  // =========================================================================
  // Before doing ANYTHING, take the physical I2C pins and clamp them to ground.
  // This drains any parasitic voltage the touch chip is stealing from the long lines.
  pinMode(18, OUTPUT);
  digitalWrite(18, LOW);  // Force SDA to Ground
  pinMode(19, OUTPUT);
  digitalWrite(19, LOW);  // Force SCL to Ground
  pinMode(9, OUTPUT);
  digitalWrite(9, LOW);  // Force LCD RST to Ground

  // Hold everything hard-grounded for 350ms to completely clear the lines
  delay(350);

  // Release the lines back to input states before drivers execute
  pinMode(18, INPUT);
  pinMode(19, INPUT);
  pinMode(9, INPUT);

  // Resume standard boot timing buffer
  delay(500);
  Serial.begin(115200);

  initAudioSystem();

  bool systemReady = false;
  int retryCount = 0;

  while (!systemReady && retryCount < 5) {
    initDisplaySystem();

    drawBootLoadingScreen();
    tft.setCursor(180, 185);
    tft.setTextColor(COLOR_RAMS_ORANGE);
    tft.setTextSize(1);

    if (retryCount > 0) {
      tft.printf("SYSTEM RECOVERY: ATTEMPT %d/5", retryCount + 1);
    } else {
      tft.print("INITIALIZING HARDWARE...");
    }

    // Handshake Check
    Wire.beginTransmission(FT6336U_ADDR);
    if (Wire.endTransmission(true) == 0) {
      systemReady = true;
      Wire.setClock(400000);
      Serial.println("SYSTEM: Touch Hardware Recovered!");
    } else {
      Serial.printf("WARNING: Handshake Failed (Attempt %d). Retrying...\n", retryCount + 1);
      retryCount++;
      delay(400);
    }
  }

  if (!systemReady) {
    tft.fillRect(0, 180, 480, 40, COLOR_RAMS_BG);
    tft.setCursor(140, 185);
    tft.setTextColor(0xF800);
    tft.print("BROWNOUT DETECTED - DRAINING RESIDUAL POWER");

    Wire.end();
    pinMode(18, OUTPUT);
    digitalWrite(18, LOW);
    pinMode(19, OUTPUT);
    digitalWrite(19, LOW);
    delay(1500);

    RESTART_TEENSY();
  }

  if (!(SD.begin(BUILTIN_SDCARD))) {
    tft.fillScreen(COLOR_RAMS_BG);
    tft.setCursor(20, 150);
    tft.setTextColor(COLOR_RAMS_ORANGE);
    tft.print("SD CARD ERROR");
    while (1) { delay(100); }
  }

  buildLibraryIndex();

  currentUIState = STATE_MENU;
  currentMenuLevel = LEVEL_ARTISTS;
  menuScrollOffset = 0;

  drawMenuScreen();
}

void loop() {
  processTouchControls();
  updateAudioEngine();

  if (isMediaPlaying && currentUIState == STATE_PLAYER) {
    handleLiveTimeAndProgressBar();
  }
  delay(2);
}
