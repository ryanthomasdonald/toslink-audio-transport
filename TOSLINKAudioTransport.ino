#include <SPI.h>
#include <SD.h>
#include "AudioEngine.h"
#include "DisplayUI.h"
#include "LibraryCommon.h"

void setup() {
  delay(1500);
  Serial.begin(115200);

  // Initialize modular systems
  initAudioSystem();
  initDisplaySystem();

  if (!(SD.begin(BUILTIN_SDCARD))) {
    Serial.println("SD Card initialization failed!");
  }

  // --- THE FIXED MAGIC COMBINATION PASS ---
  // Recursively indexes all artists, albums, and tracks into RAM at power-up
  buildLibraryIndex();

  // Force OS parameters directly into menu browser mode on page 1
  currentUIState = STATE_MENU;
  currentMenuLevel = LEVEL_ARTISTS;
  menuScrollOffset = 0;

  // Render the initial Artist browser screen cleanly out of RAM
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
