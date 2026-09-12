#include <SPI.h>
#include <SD.h>
#include "AudioEngine.h"
#include "DisplayUI.h"

void setup() {
  delay(1500);
  Serial.begin(115200);

  // Initialize modular systems
  initAudioSystem();
  initDisplaySystem();

  if (!(SD.begin(BUILTIN_SDCARD))) {
    Serial.println("SD Card initialization failed!");
  }

  // Scan folder contents dynamically at boot
  scanCurrentAlbumFolder();

  // Initial UI Render Pass
  drawAudioDashboard();
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
