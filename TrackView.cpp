#include "LibraryCommon.h"
#include "DisplayUI.h"
#include "AudioEngine.h"

static String persistArtistPath = "";
static String persistAlbumPath = "";

void drawTrackView() {
  tft.fillScreen(COLOR_RAMS_BG);

  // 1. Structural Header Block
  tft.fillRect(0, 0, 480, 45, COLOR_RAMS_CARD);
  tft.drawFastHLine(0, 45, 480, COLOR_RAMS_DIVIDER);
  tft.setTextColor(COLOR_RAMS_ORANGE);
  tft.setTextSize(2);
  tft.setCursor(15, 15);

  String albumHeader = String(library[selectedArtistIndex].albums[selectedAlbumIndex].name);
  albumHeader.toUpperCase();
  tft.print(albumHeader.c_str());

  // 2. PERFECT FULL-SCREEN GRID SIDE NAVIGATION STACK (Flush & Touching)
  // BACK Button (Exactly 92px tall)
  tft.fillRect(340, 45, 140, 92, COLOR_RAMS_CARD);
  tft.drawRect(340, 45, 140, 92, COLOR_RAMS_DIVIDER);
  tft.setTextColor(COLOR_RAMS_WHITE);
  tft.setTextSize(2);
  tft.setCursor(385, 81);
  tft.print("BACK");

  // PG UP Button (Exactly 92px tall)
  tft.fillRect(340, 137, 140, 92, COLOR_RAMS_CARD);
  tft.drawRect(340, 137, 140, 92, COLOR_RAMS_DIVIDER);
  tft.setCursor(380, 173);
  tft.print("PG UP");

  // PG DN Button (Exactly 91px tall to hit bottom edge at 320)
  tft.fillRect(340, 229, 140, 91, COLOR_RAMS_CARD);
  tft.drawRect(340, 229, 140, 91, COLOR_RAMS_DIVIDER);
  tft.setCursor(380, 265);
  tft.print("PG DN");

  // 3. PIXEL-PERFECT LIST LAYER (5 items @ 55px completely fills 45 to 320)
  int trackCount = library[selectedArtistIndex].albums[selectedAlbumIndex].trackCount;
  if (trackCount == 0) {
    tft.setTextColor(COLOR_RAMS_TEXT_MUTE);
    tft.setCursor(30, 120);
    tft.print("EMPTY");
  } else {
    for (int i = 0; i < 5; i++) {
      int itemIndex = menuScrollOffset + i;
      int boxY = 45 + (i * 55);

      if (itemIndex >= trackCount) {
        tft.fillRect(0, boxY, 340, 55, COLOR_RAMS_BG);
        tft.drawRect(0, boxY, 340, 55, COLOR_RAMS_DIVIDER);
        continue;
      }

      tft.fillRect(0, boxY, 340, 55, COLOR_RAMS_CARD);
      tft.drawRect(0, boxY, 340, 55, COLOR_RAMS_DIVIDER);
      tft.fillRect(10, boxY + 24, 8, 8, COLOR_RAMS_ORANGE);

      // Strip numeric sorting prefixes (e.g. "01_") and extensions
      String cleanTrack = String(library[selectedArtistIndex].albums[selectedAlbumIndex].tracks[itemIndex].filename);
      if (cleanTrack.length() > 3) cleanTrack = cleanTrack.substring(3);
      if (cleanTrack.endsWith(".wav") || cleanTrack.endsWith(".WAV")) {
        cleanTrack = cleanTrack.substring(0, cleanTrack.length() - 4);
      }
      cleanTrack.toUpperCase();

      int16_t x1, y1;
      uint16_t tw, th;
      tft.setTextSize(2);
      tft.getTextBounds(cleanTrack.c_str(), 32, boxY + 20, &x1, &y1, &tw, &th);
      while (tw > 290 && cleanTrack.length() > 4) {
        cleanTrack = cleanTrack.substring(0, cleanTrack.length() - 1);
        String testStr = cleanTrack + "...";
        tft.getTextBounds(testStr.c_str(), 32, boxY + 20, &x1, &y1, &tw, &th);
      }
      if (tw <= 290 && cleanTrack.length() < (strlen(library[selectedArtistIndex].albums[selectedAlbumIndex].tracks[itemIndex].filename) - 7)) {
        cleanTrack += "...";
      }

      tft.setTextColor(COLOR_RAMS_WHITE);
      tft.setCursor(32, boxY + 20);
      tft.print(cleanTrack.c_str());
    }
  }
}

void processTrackViewTouch() {
  static bool lastTouchState = false;
  bool currentTouch = readTouchPanel(touchX, touchY);
  int trackCount = library[selectedArtistIndex].albums[selectedAlbumIndex].trackCount;
  if (currentTouch && !lastTouchState) {
    // Navigation Column Grid-Hit Geometry
    if (touchX >= 340 && touchX <= 480) {
      if (touchY >= 45 && touchY <= 136) {  // BACK
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

    // List Content Rows Grid-Hit Geometry
    if (touchX >= 0 && touchX <= 340) {
      for (int i = 0; i < 5; i++) {
        int itemIndex = menuScrollOffset + i;
        if (itemIndex >= trackCount) break;

        int boxY = 45 + (i * 55);
        if (touchY >= boxY && touchY <= (boxY + 55)) {
          playWav1.stop();
          playWav2.stop();

          persistArtistPath = String(library[selectedArtistIndex].name) + "/";
          persistAlbumPath = String(library[selectedArtistIndex].albums[selectedAlbumIndex].name) + "/";

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
          updatePlayPauseButtonLabel("||", COLOR_RAMS_CARD);
          playFreshAlbumStart();
          break;
        }
      }
    }
  }
  lastTouchState = currentTouch;
}
