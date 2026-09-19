#include "LibraryCommon.h"
#include "DisplayUI.h"
#include "AudioEngine.h"

// 🚀 EXPLICIT LAYER EXPOSURE
extern int browseArtistIndex;
extern int browseAlbumIndex;

void drawTrackView() {
  refreshDisplayHardwareState();
  tft.fillScreen(COLOR_RAMS_BG);
  int dummyNextY = 0;

  tft.fillRect(0, 0, 480, 45, COLOR_RAMS_CARD);
  tft.drawFastHLine(0, 45, 480, COLOR_RAMS_DIVIDER);
  drawWrappedTextLine("SELECT TRACK", 15, 19, 320, 2, COLOR_RAMS_ORANGE, COLOR_RAMS_CARD, 0, 1, dummyNextY);

  tft.fillRect(360, 45, 120, 92, COLOR_RAMS_CARD);
  tft.drawRect(360, 45, 120, 92, COLOR_RAMS_DIVIDER);
  drawWrappedTextLine("BACK", 396, 81, 100, 2, COLOR_RAMS_WHITE, COLOR_RAMS_CARD, 0, 1, dummyNextY);

  int midX = 360 + (120 / 2);
  int midY1 = 137 + (92 / 2);
  tft.fillRect(360, 137, 120, 92, COLOR_RAMS_CARD);
  tft.drawRect(360, 137, 120, 92, COLOR_RAMS_DIVIDER);
  tft.fillTriangle(midX, midY1 - 10, midX - 12, midY1 + 6, midX + 12, midY1 + 6, COLOR_RAMS_WHITE);

  int midY2 = 229 + (91 / 2);
  tft.fillRect(360, 229, 120, 91, COLOR_RAMS_CARD);
  tft.drawRect(360, 229, 120, 91, COLOR_RAMS_DIVIDER);
  tft.fillTriangle(midX, midY2 + 10, midX - 12, midY2 - 6, midX + 12, midY2 - 6, COLOR_RAMS_WHITE);

  // 🚀 THE REDIRECT FIX: Target browse registers to look up track entries safely
  int trackCount = library[browseArtistIndex].albums[browseAlbumIndex].trackCount;

  if (trackCount == 0) {
    drawWrappedTextLine("EMPTY", 30, 120, 200, 2, COLOR_RAMS_TEXT_MUTE, COLOR_RAMS_BG, 0, 1, dummyNextY);
  } else {
    for (int i = 0; i < 5; i++) {
      int itemIndex = menuScrollOffset + i;
      int boxY = 45 + (i * 55);

      if (itemIndex >= trackCount) {
        tft.fillRect(0, boxY, 360, 55, COLOR_RAMS_BG);
        tft.drawRect(0, boxY, 360, 55, COLOR_RAMS_DIVIDER);
        continue;
      }

      tft.fillRect(0, boxY, 360, 55, COLOR_RAMS_CARD);
      tft.drawRect(0, boxY, 360, 55, COLOR_RAMS_DIVIDER);
      tft.fillRect(12, boxY + 24, 8, 8, COLOR_RAMS_ORANGE);

      String nameStr = "";
      if (library[browseArtistIndex].albums[browseAlbumIndex].tracks[itemIndex].filename != NULL) {
        nameStr = String(library[browseArtistIndex].albums[browseAlbumIndex].tracks[itemIndex].filename);
      }
      if (nameStr.length() > 3) nameStr = nameStr.substring(3);
      if (nameStr.endsWith(".wav") || nameStr.endsWith(".WAV")) { nameStr = nameStr.substring(0, nameStr.length() - 4); }
      nameStr.toUpperCase();
      drawWrappedTextLine(nameStr.c_str(), 34, boxY + 22, 310, 2, COLOR_RAMS_WHITE, COLOR_RAMS_CARD, 0, 1, dummyNextY);
    }
  }
}

void processTrackViewTouch() {
  static bool lastTouchState = false;
  bool currentTouch = readTouchPanel(touchX, touchY);
  int trackCount = library[browseArtistIndex].albums[browseAlbumIndex].trackCount;

  if (currentTouch && !lastTouchState) {
    if (touchX >= 360 && touchX <= 480) {
      if (touchY >= 45 && touchY <= 136) {  // BACK BUTTON
        menuScrollOffset = 0;
        currentMenuLevel = LEVEL_ALBUMS;
        drawAlbumView();
      } else if (touchY >= 137 && touchY <= 228) {  // PG UP
        if (menuScrollOffset >= 5) {
          menuScrollOffset -= 5;
          drawTrackView();
        }
      } else if (touchY >= 229 && touchY <= 320) {  // PG DN
        if (menuScrollOffset + 5 < trackCount) {
          menuScrollOffset += 5;
          drawTrackView();
        }
      }
    }

    if (touchX >= 0 && touchX <= 360) {
      for (int i = 0; i < 5; i++) {
        int itemIndex = menuScrollOffset + i;
        if (itemIndex >= trackCount) break;
        int boxY = 45 + (i * 55);
        if (touchY >= boxY && touchY <= (boxY + 55)) {

          isMediaPlaying = false;
          isMediaPaused = false;

          extern EngineLifecycleState activeEngineState;
          activeEngineState = ENGINE_IDLE;
          delay(10);

          // 🚀 THE COMMIT INTERLOCK: Lock browse registers into active playback metrics
          selectedArtistIndex = browseArtistIndex;
          selectedAlbumIndex = browseAlbumIndex;
          currentTrackIndex = itemIndex;

          populateTrackQueue();

          // 🚀 THE ALIGNMENT SHIELD: Construct the path with NO trailing slash!
          // This prevents double-slash path resolution failures inside SD.open()
          String folderPath = "/" + String(library[selectedArtistIndex].name) + "/" + String(library[selectedArtistIndex].albums[selectedAlbumIndex].name);
          strncpy(currentAlbumAbsolutePath, folderPath.c_str(), sizeof(currentAlbumAbsolutePath) - 1);
          currentAlbumAbsolutePath[sizeof(currentAlbumAbsolutePath) - 1] = '\0';

          if (library[selectedArtistIndex].name != NULL) {
            snprintf(currentArtistFolder, sizeof(currentArtistFolder), "%s", library[selectedArtistIndex].name);
          }

          if (library[selectedArtistIndex].albums[selectedAlbumIndex].name != NULL) {
            snprintf(currentAlbumFolder, sizeof(currentAlbumFolder), "%s", library[selectedArtistIndex].albums[selectedAlbumIndex].name);
          }

          // Cache artwork with a cleanly injected separator
          String artworkPath = folderPath + "/" + String(library[selectedArtistIndex].albums[selectedAlbumIndex].artworkFilename);
          cacheActiveAlbumArtwork(artworkPath);

          currentUIState = STATE_PLAYER;
          drawAudioDashboard();
          updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);

          delay(150);

          // 🚀 ENGAGE UNIFIED BUFFER SYSTEM CYCLE: Clear RAM, strip headers, play music!
          playFreshAlbumStart();
          updatePlayPauseButtonLabel("||", COLOR_RAMS_CARD);
          break;
        }
      }
    }
  }
  lastTouchState = currentTouch;
}
