#include "DisplayUI.h"
#include "LibraryMenu.h"
#include "AudioEngine.h"
#include <SD.h>
#include <Audio.h>  // Crucial for real-time interrupt safeguards

UIState currentUIState = STATE_PLAYER;
MenuLevel currentMenuLevel = LEVEL_ARTISTS;

ArtistEntry library[MAX_ARTISTS_TOTAL];
int libraryArtistCount = 0;

int selectedArtistIndex = -1;
int selectedAlbumIndex = -1;
int menuScrollOffset = 0;

// The single, dedicated dynamic single image cache frame buffer (RAM2)
DMAMEM uint16_t activeArtworkCache[160 * 160];
bool activeArtworkLoaded = false;

static String persistArtistPath = "";
static String persistAlbumPath = "";

bool readTouchPanel(uint16_t& x, uint16_t& y);

// -----------------------------------------------------------------------------
// 📁 COHESIVE DIRECTORY SCANNING MECHANICS WITH ZERO-STACK-OVERFLOW SORT ARRAYS
// -----------------------------------------------------------------------------

void scanRootForArtists() {
  AudioNoInterrupts();  // 🔒 PROTECT BUS FROM AUDIO INTERRUPT INTERFERENCE

  libraryArtistCount = 0;
  currentMenuLevel = LEVEL_ARTISTS;

  File root = SD.open("/");
  if (!root) {
    AudioInterrupts();
    return;
  }

  while (true) {
    File artistDir = root.openNextFile();
    if (!artistDir) break;

    if (artistDir.isDirectory()) {
      String artistName = String(artistDir.name());

      // Strict filter configuration: Skip hidden files and Windows system files
      if (!artistName.startsWith(".") && artistName != "System Volume Information" && libraryArtistCount < MAX_ARTISTS_TOTAL) {

        ArtistEntry* artist = &library[libraryArtistCount];
        artist->name = strdup(artistName.c_str());
        artist->albumCount = 0;
        libraryArtistCount++;
      }
    }
    artistDir.close();
  }
  root.close();

  // 🚀 FIXED: Zero-Stack Alphabetical Sort using pointer address swaps
  for (int i = 0; i < libraryArtistCount - 1; i++) {
    for (int j = i + 1; j < libraryArtistCount; j++) {
      if (strcmp(library[i].name, library[j].name) > 0) {
        // Swap names and counts safely without copying the whole heavy structure nested arrays
        char* tempName = library[i].name;
        library[i].name = library[j].name;
        library[j].name = tempName;

        int tempCount = library[i].albumCount;
        library[i].albumCount = library[j].albumCount;
        library[j].albumCount = tempCount;

        // Swap the internal sub-structures cleanly via shallow memory mapping
        for (int a = 0; a < MAX_ALBUMS_PER_ARTIST; a++) {
          AlbumEntry tempAlbum = library[i].albums[a];
          library[i].albums[a] = library[j].albums[a];
          library[j].albums[a] = tempAlbum;
        }
      }
    }
  }

  AudioInterrupts();  // 🔓 SAFELY RE-ENGAGE CORE PIPELINES
}

void scanArtistForAlbums(String artist) {
  AudioNoInterrupts();  // 🔒 LOCK BUS

  if (selectedArtistIndex == -1) {
    AudioInterrupts();
    return;
  }
  ArtistEntry* actArtist = &library[selectedArtistIndex];
  actArtist->albumCount = 0;
  currentMenuLevel = LEVEL_ALBUMS;

  String fullPath = "/" + artist;
  File dir = SD.open(fullPath.c_str());
  if (!dir) {
    AudioInterrupts();
    return;
  }

  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;

    if (entry.isDirectory()) {
      String name = String(entry.name());
      if (!name.startsWith(".") && actArtist->albumCount < MAX_ALBUMS_PER_ARTIST) {
        AlbumEntry* album = &actArtist->albums[actArtist->albumCount];
        album->name = strdup(name.c_str());
        album->trackCount = 0;
        album->artworkFilename = NULL;
        actArtist->albumCount++;
      }
    }
    entry.close();
  }
  dir.close();

  // 🚀 FIXED: Zero-Stack Safe Alphabetical Album Sort
  for (int i = 0; i < actArtist->albumCount - 1; i++) {
    for (int j = i + 1; j < actArtist->albumCount; j++) {
      if (strcmp(actArtist->albums[i].name, actArtist->albums[j].name) > 0) {
        AlbumEntry temp = actArtist->albums[i];
        actArtist->albums[i] = actArtist->albums[j];
        actArtist->albums[j] = temp;
      }
    }
  }

  AudioInterrupts();  // 🔓 UNLOCK BUS
}

void scanAlbumForTracks(String artist, String album) {
  AudioNoInterrupts();  // 🔒 LOCK BUS

  if (selectedArtistIndex == -1 || selectedAlbumIndex == -1) {
    AudioInterrupts();
    return;
  }
  AlbumEntry* actAlbum = &library[selectedArtistIndex].albums[selectedAlbumIndex];
  actAlbum->trackCount = 0;
  currentMenuLevel = LEVEL_TRACKS;

  String fullPath = "/" + artist + "/" + album;
  File dir = SD.open(fullPath.c_str());
  if (!dir) {
    AudioInterrupts();
    return;
  }

  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;

    String name = String(entry.name());
    if (!name.startsWith(".")) {
      if (!entry.isDirectory() && (name.endsWith(".wav") || name.endsWith(".WAV"))) {
        if (actAlbum->trackCount < MAX_TRACKS_PER_ALBUM) {
          actAlbum->tracks[actAlbum->trackCount].filename = strdup(name.c_str());
          actAlbum->trackCount++;
        }
      } else if (!entry.isDirectory() && (name.endsWith(".bmp") || name.endsWith(".BMP"))) {
        if (actAlbum->artworkFilename == NULL) {
          actAlbum->artworkFilename = strdup(name.c_str());
        }
      }
    }
    entry.close();
  }
  dir.close();

  // 🚀 FIXED: Zero-Stack Safe Alphabetical Track Sort
  for (int i = 0; i < actAlbum->trackCount - 1; i++) {
    for (int j = i + 1; j < actAlbum->trackCount; j++) {
      if (strcmp(actAlbum->tracks[i].filename, actAlbum->tracks[j].filename) > 0) {
        TrackEntry temp = actAlbum->tracks[i];
        actAlbum->tracks[i] = actAlbum->tracks[j];
        actAlbum->tracks[j] = temp;
      }
    }
  }

  AudioInterrupts();  // 🔓 UNLOCK BUS
}

void cacheActiveAlbumArtwork(String path) {
  AudioNoInterrupts();  // 🔒 LOCK BUS WHILE EXTRACTING BITMAP ROWS

  activeArtworkLoaded = false;
  File bmpFile = SD.open(path.c_str(), FILE_READ);
  if (!bmpFile) {
    AudioInterrupts();
    return;
  }

  uint32_t dataOffset = 54;
  bmpFile.seek(10);
  bmpFile.read((uint8_t*)&dataOffset, 4);

  for (int y = 159; y >= 0; y--) {
    bmpFile.seek(dataOffset + (y * 320));
    bmpFile.read((uint8_t*)&activeArtworkCache[(159 - y) * 160], 320);
  }
  bmpFile.close();
  activeArtworkLoaded = true;

  AudioInterrupts();  // 🔓 UNLOCK BUS
}

void drawMenuSideButton(int x, int y, int w, int h, const char* label, uint16_t color) {
  tft.fillRoundRect(x, y, w, h, 10, color);
  tft.drawRoundRect(x, y, w, h, 10, ST7735_WHITE);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(2);

  int16_t x1, y1;
  uint16_t tw, th;
  tft.getTextBounds(label, x, y, &x1, &y1, &tw, &th);
  tft.setCursor(x + (w - tw) / 2, y + (h - th) / 2 + 4);
  tft.print(label);
}

void drawMenuScreen() {
  tft.fillScreen(0x0010);
  tft.fillRect(0, 0, 480, 45, 0x2104);
  tft.drawFastHLine(0, 45, 480, ST7735_WHITE);
  tft.setTextColor(ST7735_CYAN);
  tft.setTextSize(2);
  tft.setCursor(15, 13);

  int itemCount = 0;
  if (currentMenuLevel == LEVEL_ARTISTS) {
    tft.print("SELECT ARTIST");
    itemCount = libraryArtistCount;
  } else if (currentMenuLevel == LEVEL_ALBUMS) {
    tft.print(library[selectedArtistIndex].name);
    itemCount = library[selectedArtistIndex].albumCount;
  } else if (currentMenuLevel == LEVEL_TRACKS) {
    tft.print(library[selectedArtistIndex].albums[selectedAlbumIndex].name);
    itemCount = library[selectedArtistIndex].albums[selectedAlbumIndex].trackCount;
  }

  drawMenuSideButton(370, 60, 95, 65, "BACK", ST7735_RED);
  drawMenuSideButton(370, 140, 95, 65, "PG UP", 0x3186);
  drawMenuSideButton(370, 220, 95, 65, "PG DN", 0x3186);

  int itemYOffset = 60;
  tft.setTextSize(2);

  if (itemCount == 0) {
    tft.setTextColor(0x7BEF);
    tft.setCursor(30, 100);
    tft.print("EMPTY");
  } else {
    for (int i = 0; i < 5; i++) {
      int itemIndex = menuScrollOffset + i;
      if (itemIndex >= itemCount) break;
      int itemBoxY = itemYOffset + (i * 50);
      tft.fillRect(15, itemBoxY - 4, 340, 42, 0x10A2);
      tft.drawRoundRect(15, itemBoxY - 4, 340, 42, 4, 0x3186);
      tft.fillRect(25, itemBoxY + 13, 8, 8, (currentMenuLevel == LEVEL_TRACKS) ? ST7735_GREEN : 0x5AAA);
      tft.setTextColor(ST7735_WHITE);
      tft.setCursor(45, itemBoxY + 9);

      char* labelText;
      if (currentMenuLevel == LEVEL_ARTISTS) labelText = library[itemIndex].name;
      else if (currentMenuLevel == LEVEL_ALBUMS) labelText = library[selectedArtistIndex].albums[itemIndex].name;
      else labelText = library[selectedArtistIndex].albums[selectedAlbumIndex].tracks[itemIndex].filename;

      if (currentMenuLevel == LEVEL_TRACKS) {
        String cleanTrack = String(labelText);
        if (cleanTrack.length() > 3) cleanTrack = cleanTrack.substring(3);
        if (cleanTrack.endsWith(".wav") || cleanTrack.endsWith(".WAV")) cleanTrack = cleanTrack.substring(0, cleanTrack.length() - 4);
        tft.print(cleanTrack.c_str());
      } else {
        tft.print(labelText);
      }
    }
  }
}

void processMenuTouch() {
  static bool lastTouchStateMenu = false;
  bool currentTouch = readTouchPanel(touchX, touchY);

  if (currentTouch && !lastTouchStateMenu) {
    int totalLimit = 0;
    if (currentMenuLevel == LEVEL_ARTISTS) totalLimit = libraryArtistCount;
    else if (currentMenuLevel == LEVEL_ALBUMS) totalLimit = library[selectedArtistIndex].albumCount;
    else totalLimit = library[selectedArtistIndex].albums[selectedAlbumIndex].trackCount;

    if (touchX >= 370 && touchX <= 465) {
      if (touchY >= 60 && touchY <= 125) {
        menuScrollOffset = 0;
        if (currentMenuLevel == LEVEL_TRACKS) {
          scanArtistForAlbums(library[selectedArtistIndex].name);
          drawMenuScreen();
        } else if (currentMenuLevel == LEVEL_ALBUMS) {
          scanRootForArtists();
          drawMenuScreen();
        } else {
          currentUIState = STATE_PLAYER;
          drawAudioDashboard();
          extern uint32_t lastUpdatedSecond;
          extern uint16_t lastProgressPixelWidth;
          lastUpdatedSecond = 999999;
          lastProgressPixelWidth = 0;
          updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);
          if (isMediaPlaying) updatePlayPauseButtonLabel("PAUSE", 0xD4A0);
          else updatePlayPauseButtonLabel("PLAY", 0x03E0);
        }
      } else if (touchY >= 140 && touchY <= 205) {
        if (menuScrollOffset >= 5) {
          menuScrollOffset -= 5;
          drawMenuScreen();
        }
      } else if (touchY >= 220 && touchY <= 285) {
        if (menuScrollOffset + 5 < totalLimit) {
          menuScrollOffset += 5;
          drawMenuScreen();
        }
      }
    }
    int itemYOffset = 60;
    for (int i = 0; i < 5; i++) {
      int itemIndex = menuScrollOffset + i;
      if (itemIndex >= totalLimit) break;
      int itemBoxY = itemYOffset + (i * 50);
      if (touchX >= 15 && touchX <= 355 && touchY >= (itemBoxY - 4) && touchY <= (itemBoxY + 38)) {
        if (currentMenuLevel == LEVEL_ARTISTS) {
          selectedArtistIndex = itemIndex;
          menuScrollOffset = 0;
          scanArtistForAlbums(library[itemIndex].name);
          drawMenuScreen();
        } else if (currentMenuLevel == LEVEL_ALBUMS) {
          selectedAlbumIndex = itemIndex;
          menuScrollOffset = 0;
          scanAlbumForTracks(library[selectedArtistIndex].name, library[selectedArtistIndex].albums[itemIndex].name);
          drawMenuScreen();
        } else if (currentMenuLevel == LEVEL_TRACKS) {
          playWav1.stop();
          playWav2.stop();
          persistArtistPath = String(library[selectedArtistIndex].name) + "/";
          persistAlbumPath = String(library[selectedArtistIndex].albums[selectedAlbumIndex].name) + "/";
          currentArtistFolder = persistArtistPath.c_str();
          currentAlbumFolder = persistAlbumPath.c_str();
          AlbumEntry* activeAlbum = &library[selectedArtistIndex].albums[selectedAlbumIndex];
          if (activeAlbum->artworkFilename != NULL) {
            String targetImgPath = String("/") + String(library[selectedArtistIndex].name) + "/" + String(activeAlbum->name) + "/" + String(activeAlbum->artworkFilename);
            cacheActiveAlbumArtwork(targetImgPath);
          } else {
            activeArtworkLoaded = false;
          }
          scanCurrentAlbumFolder();
          currentTrackIndex = itemIndex;
          currentUIState = STATE_PLAYER;
          drawAudioDashboard();
          playFreshAlbumStart();
        }
        break;
      }
    }
  }
  lastTouchStateMenu = currentTouch;
}