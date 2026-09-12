#include <SPI.h>
#include <SD.h>
#include "AudioEngine.h"
#include "DisplayUI.h"
#include "LibraryMenu.h"

void setup() {
  delay(1500);
  Serial.begin(115200);

  initAudioSystem();
  initDisplaySystem();

  if (!(SD.begin(BUILTIN_SDCARD))) {
    Serial.println("SD Card initialization failed!");
  }

  // --- THE NEW MAGIC LINE ---
  buildLibraryIndex(); // Scans everything once into RAM
  
  // Default load (optional, or you can start with empty queue)
  scanCurrentAlbumFolder(); 
  drawAudioDashboard();
}

void loop() {
  processTouchControls();
  updateAudioEngine();
  if (isMediaPlaying) {
    handleLiveTimeAndProgressBar();
  }
  delay(2);
}
