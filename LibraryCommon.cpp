#include "LibraryCommon.h"
#include "DisplayUI.h"
#include "AudioEngine.h"
#include <SD.h>
#include <Audio.h>

UIState currentUIState = STATE_MENU;
MenuLevel currentMenuLevel = LEVEL_ARTISTS;
ArtistEntry library[MAX_ARTISTS_TOTAL];
int libraryArtistCount = 0;
int selectedArtistIndex = -1;
int selectedAlbumIndex = -1;
int menuScrollOffset = 0;

// 🚀 MEMORY BLOCK RESIZED TO 78.12 KB FOR THE NEW 200x200 CANVAS SIZE
DMAMEM uint16_t activeArtworkCache[200 * 200];
bool activeArtworkLoaded = false;

static String persistArtistPath = "";
static String persistAlbumPath = "";

bool readTouchPanel(uint16_t& x, uint16_t& y);

void buildLibraryIndex() {
  AudioNoInterrupts();
  Serial.println("Deep Indexing SD card structure into RAM...");
  libraryArtistCount = 0;
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
      if (!artistName.startsWith(".") && artistName != "System Volume Information" && libraryArtistCount < MAX_ARTISTS_TOTAL) {
        ArtistEntry* artist = &library[libraryArtistCount];
        artist->name = strdup(artistName.c_str());
        artist->albumCount = 0;
        artist->albums = NULL;

        String artistPath = "/" + artistName;
        File albumsDir = SD.open(artistPath.c_str());
        if (albumsDir) {
          while (true) {
            File albumDir = albumsDir.openNextFile();
            if (!albumDir) break;
            if (albumDir.isDirectory()) {
              String albumName = String(albumDir.name());
              if (!albumName.startsWith(".") && artist->albumCount < MAX_ALBUMS_PER_ARTIST) {
                artist->albums = (AlbumEntry*)realloc(artist->albums, (artist->albumCount + 1) * sizeof(AlbumEntry));
                AlbumEntry* album = &artist->albums[artist->albumCount];
                album->name = strdup(albumName.c_str());
                album->trackCount = 0;
                album->tracks = NULL;
                album->artworkFilename = NULL;

                String fullAlbumPath = artistPath + "/" + albumName;
                File tracksDir = SD.open(fullAlbumPath.c_str());
                if (tracksDir) {
                  while (true) {
                    File trackFile = tracksDir.openNextFile();
                    if (!trackFile) break;
                    String name = String(trackFile.name());
                    if (!name.startsWith(".")) {
                      if (!trackFile.isDirectory() && (name.endsWith(".wav") || name.endsWith(".WAV"))) {
                        if (album->trackCount < MAX_TRACKS_PER_ALBUM) {
                          album->tracks = (TrackEntry*)realloc(album->tracks, (album->trackCount + 1) * sizeof(TrackEntry));
                          album->tracks[album->trackCount].filename = strdup(name.c_str());
                          album->trackCount++;
                        }
                      } else if (!trackFile.isDirectory() && (name.endsWith(".bmp") || name.endsWith(".BMP"))) {
                        if (album->artworkFilename == NULL) {
                          album->artworkFilename = strdup(name.c_str());
                        }
                      }
                    }
                    trackFile.close();
                  }
                  tracksDir.close();
                }

                for (int t1 = 0; t1 < album->trackCount - 1; t1++) {
                  for (int t2 = t1 + 1; t2 < album->trackCount; t2++) {
                    if (strcmp(album->tracks[t1].filename, album->tracks[t2].filename) > 0) {
                      TrackEntry temp = album->tracks[t1];
                      album->tracks[t1] = album->tracks[t2];
                      album->tracks[t2] = temp;
                    }
                  }
                }
                artist->albumCount++;
              }
            }
            albumDir.close();
          }
          albumsDir.close();
        }

        for (int a1 = 0; a1 < artist->albumCount - 1; a1++) {
          for (int a2 = a1 + 1; a2 < artist->albumCount; a2++) {
            if (strcmp(artist->albums[a1].name, artist->albums[a2].name) > 0) {
              AlbumEntry temp = artist->albums[a1];
              artist->albums[a1] = artist->albums[a2];
              artist->albums[a2] = temp;
            }
          }
        }
        libraryArtistCount++;
      }
    }
    artistDir.close();
  }
  root.close();

  for (int i = 0; i < libraryArtistCount - 1; i++) {
    for (int j = i + 1; j < libraryArtistCount; j++) {
      if (strcmp(library[i].name, library[j].name) > 0) {
        char* tempName = library[i].name;
        library[i].name = library[j].name;
        library[j].name = tempName;
        int tempCount = library[i].albumCount;
        library[i].albumCount = library[j].albumCount;
        library[j].albumCount = tempCount;
        AlbumEntry* tempAlbums = library[i].albums;
        library[i].albums = library[j].albums;
        library[j].albums = tempAlbums;
      }
    }
  }
  Serial.printf("Deep Indexing Complete. Cached %d Artists in RAM.\n", libraryArtistCount);
  AudioInterrupts();
}

void cacheActiveAlbumArtwork(String path) {
  AudioNoInterrupts();
  activeArtworkLoaded = false;
  File bmpFile = SD.open(path.c_str(), FILE_READ);
  if (!bmpFile) {
    AudioInterrupts();
    return;
  }
  uint32_t dataOffset = 54;
  bmpFile.seek(10);
  bmpFile.read((uint8_t*)&dataOffset, 4);

  // 🚀 READ MATH EXTENDED TO CAPTURE THE EXPANDED 200x200 GRID DEPTH
  for (int y = 199; y >= 0; y--) {
    bmpFile.seek(dataOffset + (y * 400));  // 200 pixels * 2 bytes per RGB565 pixel = 400 bytes wide stride
    bmpFile.read((uint8_t*)&activeArtworkCache[(199 - y) * 200], 400);
  }
  bmpFile.close();
  activeArtworkLoaded = true;
  AudioInterrupts();
}

void drawMenuSideButton(int x, int y, int w, int h, const char* label, uint16_t color) {
  // Legacy helper preserved purely to prevent linker faults elsewhere
}

void drawMenuScreen() {
  if (currentMenuLevel == LEVEL_ARTISTS) drawArtistView();
  else if (currentMenuLevel == LEVEL_ALBUMS) drawAlbumView();
  else if (currentMenuLevel == LEVEL_TRACKS) drawTrackView();
}

void processMenuTouch() {
  if (currentMenuLevel == LEVEL_ARTISTS) processArtistViewTouch();
  else if (currentMenuLevel == LEVEL_ALBUMS) processAlbumViewTouch();
  else if (currentMenuLevel == LEVEL_TRACKS) processTrackViewTouch();
}
