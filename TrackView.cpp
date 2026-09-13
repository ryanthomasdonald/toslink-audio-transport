#include "LibraryCommon.h"
#include "DisplayUI.h"
#include "AudioEngine.h"

static String persistArtistPath = "";
static String persistAlbumPath = "";

void drawTrackView() {
  tft.fillScreen(0x0010);
  tft.fillRect(0, 0, 480, 45, 0x2104);
  tft.drawFastHLine(0, 45, 480, ST7735_WHITE);
  tft.setTextColor(ST7735_CYAN);
  tft.setTextSize(2);
  tft.setCursor(15, 13);
  tft.print(library[selectedArtistIndex].albums[selectedAlbumIndex].name);
  drawMenuSideButton(370, 60, 95, 65, "BACK", ST7735_RED);
  drawMenuSideButton(370, 140, 95, 65, "PG UP", 0x3186);
  drawMenuSideButton(370, 220, 95, 65, "PG DN", 0x3186);

  int trackCount = library[selectedArtistIndex].albums[selectedAlbumIndex].trackCount;
  if (trackCount == 0) {
    tft.setTextColor(0x7BEF);
    tft.setCursor(30, 100);
    tft.print("EMPTY");
  } else {
    for (int i = 0; i < 5; i++) {
      int itemIndex = menuScrollOffset + i;
      if (itemIndex >= trackCount) break;
      int itemBoxY = 60 + (i * 50);
      tft.fillRect(15, itemBoxY - 4, 340, 42, 0x10A2);
      tft.drawRoundRect(15, itemBoxY - 4, 340, 42, 4, 0x3186);
      tft.fillRect(25, itemBoxY + 13, 8, 8, ST7735_GREEN);
      tft.setTextColor(ST7735_WHITE);
      tft.setCursor(45, itemBoxY + 9);
      String cleanTrack = String(library[selectedArtistIndex].albums[selectedAlbumIndex].tracks[itemIndex].filename);
      if (cleanTrack.length() > 3) cleanTrack = cleanTrack.substring(3);
      if (cleanTrack.endsWith(".wav") || cleanTrack.endsWith(".WAV")) cleanTrack = cleanTrack.substring(0, cleanTrack.length() - 4);
      tft.print(cleanTrack.c_str());
    }
  }
}

void processTrackViewTouch() {
  static bool lastTouchState = false;
  bool currentTouch = readTouchPanel(touchX, touchY);
  int trackCount = library[selectedArtistIndex].albums[selectedAlbumIndex].trackCount;
  if (currentTouch && !lastTouchState) {
    if (touchX >= 370 && touchX <= 465) {
      if (touchY >= 60 && touchY <= 125) {
        menuScrollOffset = 0;
        currentMenuLevel = LEVEL_ALBUMS;
        drawAlbumView();
      } else if (touchY >= 140 && touchY <= 205) {
        if (menuScrollOffset >= 5) {
          menuScrollOffset -= 5;
          drawTrackView();
        }
      } else if (touchY >= 220 && touchY <= 285) {
        if (menuScrollOffset + 5 < trackCount) {
          menuScrollOffset += 5;
          drawTrackView();
        }
      }
    }
    for (int i = 0; i < 5; i++) {
      int itemIndex = menuScrollOffset + i;
      if (itemIndex >= trackCount) break;
      int itemBoxY = 60 + (i * 50);
      if (touchX >= 15 && touchX <= 355 && touchY >= (itemBoxY - 4) && touchY <= (itemBoxY + 38)) {
        playWav1.stop();
        playWav2.stop();
        persistArtistPath = String(library[selectedArtistIndex].name) + "/";
        persistAlbumPath = String(library[selectedArtistIndex].albums[selectedAlbumIndex].name) + "/";

        // 🛠️ FIX: Safely copy data strings directly into our static global buffers
        strncpy(currentArtistFolder, persistArtistPath.c_str(), PATH_BUFFER_SIZE - 1);
        currentArtistFolder[PATH_BUFFER_SIZE - 1] = '\0';
        strncpy(currentAlbumFolder, persistAlbumPath.c_str(), PATH_BUFFER_SIZE - 1);
        currentAlbumFolder[PATH_BUFFER_SIZE - 1] = '\0';

        AlbumEntry* activeAlbum = &library[selectedArtistIndex].albums[selectedAlbumIndex];
        if (activeAlbum->artworkFilename != NULL) {
          String targetImgPath = String("/") + String(library[selectedArtistIndex].name) + "/" + String(activeAlbum->name) + "/" + String(activeAlbum->artworkFilename);
          cacheActiveAlbumArtwork(targetImgPath);
        } else {
          activeArtworkLoaded = false;
        }
        scanCurrentAlbumFolder();
        currentTrackIndex = itemIndex;
        extern UIState currentUIState;
        currentUIState = STATE_PLAYER;
        drawAudioDashboard();
        playFreshAlbumStart();
        break;
      }
    }
  }
  lastTouchState = currentTouch;
}
