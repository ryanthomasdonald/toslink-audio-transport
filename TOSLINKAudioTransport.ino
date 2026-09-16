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
  // 🚀 PHASE 1: MASSIVE SD HARDWARE POWER INTEGRITY WINDOW
  // =========================================================================
  // High-capacity 512GB SDXC cards contain internal microcontroller cores that
  // consume substantial peak current spikes upon mounting. We extend the initial
  // boot cushion to 1200ms to let your breadboard rails stabilize completely.
  delay(1200);
  Serial.begin(115200);

  // =========================================================================
  // 🚀 PHASE 2: STORAGE BUS PRIORITIZATION
  // =========================================================================
  // We initialize the high-speed 4-bit SDIO card interface FIRST. This isolates
  // the heavy electrical inrush current of mounting the 512GB file system,
  // ensuring the power rails are completely flat before the display bus wakes up.
  if (!(SD.begin(BUILTIN_SDCARD))) {
    Serial.println("CRITICAL: Built-in 512GB SD Card hardware initialization failed!");

    // Safety Fallback: Attempt a basic display wake-up to report the mount error visually
    initDisplaySystem();
    tft.fillScreen(COLOR_RAMS_BG);
    tft.setCursor(20, 150);
    tft.setTextColor(COLOR_RAMS_ORANGE);
    tft.print("SD CARD ERROR");
    while (1) { delay(100); }
  }
  Serial.println("SYSTEM: 512GB SD Card hardware initialized successfully.");

  // =========================================================================
  // 🚀 PHASE 3: SCREEN DISPLAY & AUDIO BUS SUB-SYSTEM ACTIVATION
  // =========================================================================
  initDisplaySystem();  // Starts display at 30MHz to defend against long jumper lines
  initAudioSystem();    // Instantiates native digital audio output routing

  // Immediately draw the minimal loading screen with our polished 5x5 font engine
  drawBootLoadingScreen();

  // =========================================================================
  // 🚀 PHASE 4: DEFENSIVE TOUCH BUS HANDSHAKE INTERLOCK
  // =========================================================================
  Serial.println("SYSTEM: Polling touch controller availability...");
  bool touchReady = false;
  uint32_t touchTimeoutStart = millis();

  while (!touchReady && (millis() - touchTimeoutStart < 2000)) {
    Wire.beginTransmission(FT6336U_ADDR);
    // If the touch chip replies with a clean hardware ACK (0), the bus is safe
    if (Wire.endTransmission(true) == 0) {
      touchReady = true;
      Serial.println("SYSTEM: Touch panel acknowledged I2C bus cleanly.");
    }
    delay(20);  // Minor timing delay loop to prevent voltage rail hammering
  }

  if (!touchReady) {
    Serial.println("WARNING: Touch panel initialization timed out! Operating blind.");
  }

  // =========================================================================
  // 🚀 PHASE 5: RECURSIVE FILE-TREE LIBRARY INDEXING
  // =========================================================================
  // Safely parse through your massive CD audio collection folders now that
  // both peripheral hardware power buses have fully synchronized.
  buildLibraryIndex();

  // =========================================================================
  // 🚀 PHASE 6: INITIAL STATE INTERFACE HANDOFF
  // =========================================================================
  currentUIState = STATE_MENU;
  currentMenuLevel = LEVEL_ARTISTS;
  menuScrollOffset = 0;

  // 🚀 THE DEFENSIVE SHIELD LOCK: Force an explicit hardware state refresh
  // to override any random electrical noise glitches right before drawing the menu!
  refreshDisplayHardwareState();

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
