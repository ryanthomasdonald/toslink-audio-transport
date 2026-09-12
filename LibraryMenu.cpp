#include "DisplayUI.h"   
#include "LibraryMenu.h"
#include "AudioEngine.h"
#include <SD.h>

extern UIState currentUIState;
extern uint16_t touchX;
extern uint16_t touchY;

// Master Dynamic Library Matrix
ArtistEntry library[MAX_ARTISTS_TOTAL];
int libraryArtistCount = 0;

MenuLevel currentMenuLevel = LEVEL_ARTISTS;

int selectedArtistIndex = -1;
int selectedAlbumIndex = -1;

// Buffers for the AudioEngine to read from safely
static String persistArtistPath = "";
static String persistAlbumPath = "";

bool readTouchPanel(uint16_t &x, uint16_t &y);

// ----------------------------------------------------
// 🚀 DYNAMIC INDEXER (Run Once at Boot)
// ----------------------------------------------------
void buildLibraryIndex() {
  Serial.println("Indexing SD Card dynamically...");
  libraryArtistCount = 0;
  
  File root = SD.open("/");
  if (!root) { Serial.println("SD Root Failed"); return; }

  // 1. Scan Artists (Root Folders)
  while (true) {
    File artistDir = root.openNextFile();
    if (!artistDir) break;

    if (artistDir.isDirectory()) {
      String artistName = String(artistDir.name());
      if (!artistName.startsWith(".") && libraryArtistCount < MAX_ARTISTS_TOTAL) {
        
        ArtistEntry* artist = &library[libraryArtistCount];
        artist->name = strdup(artistName.c_str());
        artist->albumCount = 0;
        
        // 2. Scan Albums (Subfolders)
        File albumsDir = SD.open((String("/") + artistName).c_str());
        while(true) {
          File albumDir = albumsDir.openNextFile();
          if (!albumDir) break;
          
          if (albumDir.isDirectory() && artist->albumCount < MAX_ALBUMS_PER_ARTIST) {
             String albumName = String(albumDir.name());
             if (!albumName.startsWith(".")) {
               
               AlbumEntry* album = &artist->albums[artist->albumCount];
               album->name = strdup(albumName.c_str());
               album->trackCount = 0;

               // 3. Scan Tracks (Files)
               String albumPath = String("/") + artistName + "/" + albumName;
               File tracksDir = SD.open(albumPath.c_str());
               while(true) {
                 File trackFile = tracksDir.openNextFile();
                 if (!trackFile) break;
                 
                 String trackName = String(trackFile.name());
                 if (!trackFile.isDirectory() && !trackName.startsWith(".") && 
                    (trackName.endsWith(".wav") || trackName.endsWith(".WAV"))) {
                      
                      if (album->trackCount < MAX_TRACKS_PER_ALBUM) {
                        album->tracks[album->trackCount].filename = strdup(trackName.c_str());
                        album->trackCount++;
                      }
                 }
                 trackFile.close();
               }
               tracksDir.close();
               
               // --- FIXED: Alphabetically sort tracks so RAM index completely mirrors AudioEngine ---
               for (int i = 0; i < album->trackCount - 1; i++) {
                 for (int j = i + 1; j < album->trackCount; j++) {
                   if (strcmp(album->tracks[i].filename, album->tracks[j].filename) > 0) {
                     TrackEntry temp = album->tracks[i];
                     album->tracks[i] = album->tracks[j];
                     album->tracks[j] = temp;
                   }
                 }
               }
               
               if (album->trackCount > 0) {
                 artist->albumCount++;
               } else {
                 free(album->name);
               }
             }
          }
          albumDir.close();
        }
        albumsDir.close();
        
        if (artist->albumCount > 0) {
          libraryArtistCount++;
        } else {
          free(artist->name);
        }
      }
    }
    artistDir.close();
  }
  root.close();
  
  // Sort Artists Alphabetically
  for (int i = 0; i < libraryArtistCount - 1; i++) {
    for (int j = i + 1; j < libraryArtistCount; j++) {
      if (strcmp(library[i].name, library[j].name) > 0) {
        ArtistEntry temp = library[i];
        library[i] = library[j];
        library[j] = temp;
      }
    }
  }

  Serial.printf("Indexing Complete. Found %d Artists allocated in memory safely.\n", libraryArtistCount);
}

// ----------------------------------------------------
// ⚡ INSTANT RAM-BASED RENDERING
// ----------------------------------------------------
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

  tft.fillRoundRect(370, 8, 95, 30, ST7735_RED, 6);
  tft.drawRoundRect(370, 8, 95, 30, 6, ST7735_WHITE);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.setCursor(402, 16);
  tft.print("BACK");

  int itemYOffset = 60;
  tft.setTextSize(2);

  if (itemCount == 0) {
    tft.setTextColor(0x7BEF);
    tft.setCursor(30, 100);
    tft.print("EMPTY");
  } else {
    for (int i = 0; i < itemCount && i < 5; i++) {
      int itemBoxY = itemYOffset + (i * 35);
      
      tft.fillRect(15, itemBoxY - 4, 450, 30, 0x10A2);
      tft.drawRoundRect(15, itemBoxY - 4, 450, 30, 4, 0x3186);
      tft.fillRect(25, itemBoxY + 7, 8, 8, (currentMenuLevel == LEVEL_TRACKS) ? ST7735_GREEN : 0x5AAA);
      tft.setTextColor(ST7735_WHITE);
      tft.setCursor(45, itemBoxY + 3);

      char* labelText;
      if (currentMenuLevel == LEVEL_ARTISTS) {
        labelText = library[i].name;
      } else if (currentMenuLevel == LEVEL_ALBUMS) {
        labelText = library[selectedArtistIndex].albums[i].name;
      } else {
        labelText = library[selectedArtistIndex].albums[selectedAlbumIndex].tracks[i].filename;
      }

      if (currentMenuLevel == LEVEL_TRACKS) {
        String cleanTrack = String(labelText);
        if (cleanTrack.length() > 3) cleanTrack = cleanTrack.substring(3);
        if (cleanTrack.endsWith(".wav") || cleanTrack.endsWith(".WAV")) {
          cleanTrack = cleanTrack.substring(0, cleanTrack.length() - 4);
        }
        tft.print(cleanTrack);
      } else {
        tft.print(labelText);
      }
    }
  }
}

// ----------------------------------------------------
// 👆 INSTANT TOUCH PROCESSING
// ----------------------------------------------------
void processMenuTouch() {
  static bool lastTouchStateMenu = false;
  bool currentTouch = readTouchPanel(touchX, touchY);

  if (currentTouch && !lastTouchStateMenu) {
    // BACK BUTTON
    if (touchX >= 370 && touchX <= 465 && touchY >= 8 && touchY <= 38) {
      if (currentMenuLevel == LEVEL_TRACKS) {
        currentMenuLevel = LEVEL_ALBUMS;
        drawMenuScreen();
      } else if (currentMenuLevel == LEVEL_ALBUMS) {
        currentMenuLevel = LEVEL_ARTISTS;
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
    }
    
    int itemYOffset = 60;
    int currentLimit = 0;
    if (currentMenuLevel == LEVEL_ARTISTS) currentLimit = libraryArtistCount;
    else if (currentMenuLevel == LEVEL_ALBUMS) currentLimit = library[selectedArtistIndex].albumCount;
    else currentLimit = library[selectedArtistIndex].albums[selectedAlbumIndex].trackCount;

    for (int i = 0; i < currentLimit && i < 5; i++) {
      int itemBoxY = itemYOffset + (i * 35);
      
      if (touchX >= 15 && touchX <= 465 && touchY >= (itemBoxY - 4) && touchY <= (itemBoxY + 26)) {
        
        if (currentMenuLevel == LEVEL_ARTISTS) {
          selectedArtistIndex = i;
          currentMenuLevel = LEVEL_ALBUMS;
          drawMenuScreen();
        } 
        else if (currentMenuLevel == LEVEL_ALBUMS) {
          selectedAlbumIndex = i;
          currentMenuLevel = LEVEL_TRACKS;
          drawMenuScreen();
        } 
        else if (currentMenuLevel == LEVEL_TRACKS) {
          playWav1.stop();
          playWav2.stop();
          
          persistArtistPath = String(library[selectedArtistIndex].name) + "/";
          persistAlbumPath  = String(library[selectedArtistIndex].albums[selectedAlbumIndex].name) + "/";
          
          currentArtistFolder = persistArtistPath.c_str();
          currentAlbumFolder  = persistAlbumPath.c_str();

          scanCurrentAlbumFolder(); 
          currentTrackIndex = i; 
          
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
