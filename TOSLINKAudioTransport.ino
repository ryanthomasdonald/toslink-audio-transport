#include <SPI.h>
#include <SD.h>
#include "AudioEngine.h"
#include "DisplayUI.h"
#include "LibraryCommon.h"

void setup() {
  // 🛠️ HARDWARE TIME INTEGRITY BOOT BLOCK
  // The Teensy 4.1 boots instantly, but external panel ICs need settling time to avoid white screens
  delay(1000);
  Serial.begin(115200);

  // Initialize modular systems in isolated, structured succession
  initDisplaySystem();  // Init panel hardware first
  initAudioSystem();    // Instantiate digital out routing

  // Explicit verification block for hardware storage interfaces
  if (!(SD.begin(BUILTIN_SDCARD))) {
    Serial.println("CRITICAL: Built-in SD Card hardware initialization failed!");
    while (1) {
      // Safe lockup warning loop
      delay(100);
    }
  }

  // Deep recursive indexing out of SD root into memory structures
  buildLibraryIndex();

  // Force clean layout execution metrics cleanly out of RAM
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
