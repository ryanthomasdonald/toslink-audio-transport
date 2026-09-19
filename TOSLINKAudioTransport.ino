#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include "AudioEngine.h"
#include "DisplayUI.h"
#include "LibraryCommon.h"

#define FT6336U_ADDR 0x38
#define RESTART_TEENSY() *(volatile uint32_t *)0xE000ED0C = 0x5FA0004

EngineLifecycleState activeEngineState = ENGINE_IDLE;
bool nextTrackPreLaunched = false;
int artistCount = 0;

// 🚀 THE UNIFICATION RESIDENCY: Host the true structural audio tracking variables right here!
volatile uint32_t ringWritePointer = 0;
volatile uint32_t ringReadPointer = 0;
volatile bool bankNeedRefill = false;
volatile int activeRefillBank = 0;

uint32_t lastTouchCheckTime = 0;
uint32_t lastUICheckTime = 0;

void setup() {
  delay(1200);
  Serial.begin(115200);

  if (!(SD.begin(BUILTIN_SDCARD))) {
    Serial.println("CRITICAL: Built-in 512GB SD Card hardware initialization failed!");
    initDisplaySystem();
    tft.fillScreen(COLOR_RAMS_BG);
    tft.setCursor(20, 150);
    tft.setTextColor(COLOR_RAMS_ORANGE);
    tft.print("SD CARD ERROR");
    while (1) { delay(100); }
  }
  Serial.println("SYSTEM: 512GB SD Card hardware initialized successfully.");

  initDisplaySystem();
  initAudioSystem();

  drawBootLoadingScreen();

  Serial.println("SYSTEM: Polling touch controller availability...");
  bool touchReady = false;
  uint32_t touchTimeoutStart = millis();

  while (!touchReady && (millis() - touchTimeoutStart < 2000)) {
    Wire.beginTransmission(FT6336U_ADDR);
    if (Wire.endTransmission(true) == 0) {
      touchReady = true;
      Serial.println("SYSTEM: Touch panel acknowledged I2C bus cleanly.");
    }
    delay(20);
  }

  buildLibraryIndex();

  tft.fillScreen(COLOR_RAMS_BG);
  refreshDisplayHardwareState();

  currentUIState = STATE_MENU;
  currentMenuLevel = LEVEL_ARTISTS;
  menuScrollOffset = 0;

  extern int browseArtistIndex;
  extern int browseAlbumIndex;
  browseArtistIndex = 0;
  browseAlbumIndex = 0;

  drawMenuScreen();

  lastTouchCheckTime = millis();
  lastUICheckTime = millis();
}

void loop() {
  // 🚀 TIER 1: CRITICAL AUDIO TRAFFIC PRIORITIZATION
  // We execute the audio engine loops continuously at raw CPU speeds with NO delays.
  updateAudioEngine();

  // 🚀 TIER 2: NON-BLOCKING TRANSPORT CONTROLS TIME-SLICE
  // We poll the I2C touch panels only once every 15 milliseconds, completely
  // preventing the touch bus overhead from stalling the audio registers!
  uint32_t currentMillis = millis();
  if (currentMillis - lastTouchCheckTime >= 15) {
    lastTouchCheckTime = currentMillis;
    processTouchControls();
  }

  // 🚀 TIER 3: NON-BLOCKING DISPLAY GRAPHICS TIME-SLICE
  // We paint the progress bars and time counters only once every 250 milliseconds.
  if (isMediaPlaying && currentUIState == STATE_PLAYER) {
    if (currentMillis - lastUICheckTime >= 250) {
      lastUICheckTime = currentMillis;
      handleLiveTimeAndProgressBar();
    }
  }
}
