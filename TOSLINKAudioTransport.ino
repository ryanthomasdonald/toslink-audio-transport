#include <SPI.h>
#include <SD.h>
#include "AudioEngine.h"
#include "DisplayUI.h"
#include "LibraryMenu.h"

void setup() {
  delay(1500);
  Serial.begin(115200);

  // Initialize modular systems
  initAudioSystem();
  initDisplaySystem();

  if (!(SD.begin(BUILTIN_SDCARD))) {
    Serial.println("SD Card initialization failed!");
  }

  // 1. --- NEW: FORCE OS TO INITIALIZE INTO BROWSER STATE ---
  currentUIState = STATE_MENU;
  currentMenuLevel = LEVEL_ARTISTS;
  menuScrollOffset = 0;

  // 2. --- NEW: RUN THE LAZY-LOAD SCAN FOR ROOT FOLDERS IMMEDIATELY ---
  scanRootForArtists();

  // 3. --- NEW: INITIAL UI RENDER PASS (Paints the Artist list right at boot) ---
  drawMenuScreen();
}

void loop() {
  // Check touch coordinates and update tracking state flags
  processTouchControls();

  // Handle background gapless data pre-loading and crossovers
  updateAudioEngine();

  // Update visual progress indicators if active stream is playing
  if (isMediaPlaying) {
    handleLiveTimeAndProgressBar();
  }

  delay(2); // 2ms optimal polling cadence resolution
}
