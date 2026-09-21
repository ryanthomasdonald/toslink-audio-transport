#include "LibraryCommon.h"
#include <SD.h>

// 🚀 THE HYBRID DOUBLE-POINTER ENGINE:
// Host our lean master lookup table root pointer inside the ultra-fast internal RAM block!
ArtistEntry* library = NULL;
int libraryArtistCount = 0;

int selectedArtistIndex = -1;
int selectedAlbumIndex = -1;

extern int currentTrackIndex;
extern int totalTracks;
extern bool activeEngineIsA;
extern char trackQueue[30][96];

char currentAlbumAbsolutePath[256] = { 0 };

void initLibrarySystem() {
  // Dynamically allocate our core artist array table pool into local internal RAM
  if (library == NULL) {
    library = (ArtistEntry*)malloc(sizeof(ArtistEntry) * MAX_ARTISTS);
    memset(library, 0, sizeof(ArtistEntry) * MAX_ARTISTS);
  }
}

// 🚀 RETAIN EXTERNAL STRING STORAGE: Keep the filename strings on the external chips
// so your internal heap capacity stays open and safe on the 512GB card!
char* allocateString(const char* src) {
  if (src == NULL) return NULL;
  char* dst = (char*)extmem_malloc(strlen(src) + 1);
  if (dst != NULL) {
    strcpy(dst, src);
  } else {
    Serial.println("❌ PSRAM ERROR: extmem_malloc out of bounds string memory space!");
  }
  return dst;
}

void buildLibraryIndex() {
  // 1. Physically construct the lean root table pointer layout inside internal memory
  initLibrarySystem();
  libraryArtistCount = 0;

  Serial.println("INDEXER: Commencing deep dynamic scan of 512GB SD storage...");

  File root = SD.open("/");
  if (!root) {
    Serial.println("INDEXER ERROR: Cannot access root directory.");
    return;
  }

  while (true) {
    File artistFolder = root.openNextFile();
    if (!artistFolder) break;  // Finished scanning root directory

    if (artistFolder.isDirectory()) {
      String folderName = String(artistFolder.name());
      String upperFolderName = folderName;
      upperFolderName.toUpperCase();

      // Robust case-insensitive check safely strips out hidden system directories
      if (upperFolderName == "SYSTEM VOLUME INFORMATION" || upperFolderName.startsWith(".")) {
        artistFolder.close();
        continue;
      }

      if (libraryArtistCount >= MAX_ARTISTS) {
        artistFolder.close();
        break;
      }

      ArtistEntry* currentArtist = &library[libraryArtistCount];
      currentArtist->name = allocateString(artistFolder.name());
      currentArtist->albumCount = 0;

      // Open the Artist's folder directory to look up Albums
      File artistDir = SD.open(("/" + String(currentArtist->name)).c_str());
      if (artistDir) {
        while (true) {
          File albumFolder = artistDir.openNextFile();
          if (!albumFolder) break;

          if (albumFolder.isDirectory()) {
            if (currentArtist->albumCount >= MAX_ALBUMS_PER_ARTIST) {
              albumFolder.close();
              break;
            }

            AlbumEntry* currentAlbum = &currentArtist->albums[currentArtist->albumCount];
            currentAlbum->name = allocateString(albumFolder.name());
            currentAlbum->trackCount = 0;
            currentAlbum->artworkFilename = allocateString("FOLDER.BMP");

            // Open the Album folder directory to extract audio files
            String fullAlbumPath = "/" + String(currentArtist->name) + "/" + String(currentAlbum->name);
            File albumDir = SD.open(fullAlbumPath.c_str());

            if (albumDir) {
              while (true) {
                File trackFile = albumDir.openNextFile();
                if (!trackFile) break;
                if (!trackFile.isDirectory()) {
                  String fname = String(trackFile.name());
                  String upperFName = fname;
                  upperFName.toUpperCase();
                  if (upperFName.endsWith(".WAV") && (fname[0] >= '0' && fname[0] <= '9')) {
                    if (currentAlbum->trackCount >= MAX_TRACKS_PER_ALBUM) {
                      trackFile.close();
                      break;
                    }
                    TrackEntry* currentTrack = &currentAlbum->tracks[currentAlbum->trackCount];
                    currentTrack->filename = allocateString(trackFile.name());
                    currentAlbum->trackCount++;
                  }
                }
                trackFile.close();
              }
              albumDir.close();
            }
            currentArtist->albumCount++;
          }
          albumFolder.close();
        }
        artistDir.close();
      }
      libraryArtistCount++;
    }
    artistFolder.close();
  }
  root.close();

  // =========================================================================
  // 🚀 ALPHABETICAL POINTER SORT ENGINES (A-Z)
  // =========================================================================
  Serial.println("INDEXER: Alphabetizing memory pointer maps for Artists...");
  for (int i = 0; i < libraryArtistCount - 1; i++) {
    for (int j = i + 1; j < libraryArtistCount; j++) {
      if (strcasecmp(library[i].name, library[j].name) > 0) {
        ArtistEntry temp = library[i];
        library[i] = library[j];
        library[j] = temp;
      }
    }
  }

  Serial.println("INDEXER: Alphabetizing memory pointer maps for Albums...");
  for (int a = 0; a < libraryArtistCount; a++) {
    for (int i = 0; i < library[a].albumCount - 1; i++) {
      for (int j = i + 1; j < library[a].albumCount; j++) {
        if (strcasecmp(library[a].albums[i].name, library[a].albums[j].name) > 0) {
          AlbumEntry tempAlbum = library[a].albums[i];
          library[a].albums[i] = library[a].albums[j];
          library[a].albums[j] = tempAlbum;
        }
      }
    }
  }

  Serial.println("INDEXER: Alphabetizing memory pointer maps for Tracks...");
  for (int a = 0; a < libraryArtistCount; a++) {
    for (int b = 0; b < library[a].albumCount; b++) {
      AlbumEntry* tgtAlbum = &library[a].albums[b];
      for (int i = 0; i < tgtAlbum->trackCount - 1; i++) {
        for (int j = i + 1; j < tgtAlbum->trackCount; j++) {
          if (strcasecmp(tgtAlbum->tracks[i].filename, tgtAlbum->tracks[j].filename) > 0) {
            TrackEntry tempTrack = tgtAlbum->tracks[i];
            tgtAlbum->tracks[i] = tgtAlbum->tracks[j];
            tgtAlbum->tracks[j] = tempTrack;
          }
        }
      }
    }
  }

  Serial.printf("INDEXER SUCCESS: Dynamic scan complete. Alphabetized %d Artists and all sub-tracks.\n", libraryArtistCount);
}

void populateTrackQueue() {
  if (selectedArtistIndex < 0 || selectedAlbumIndex < 0) {
    totalTracks = 0;
    return;
  }

  AlbumEntry* activeAlbum = &library[selectedArtistIndex].albums[selectedAlbumIndex];
  totalTracks = activeAlbum->trackCount;
  if (totalTracks > MAX_TRACKS_PER_ALBUM) totalTracks = MAX_TRACKS_PER_ALBUM;

  for (int i = 0; i < totalTracks; i++) {
    if (activeAlbum->tracks[i].filename != NULL) {
      strncpy(trackQueue[i], activeAlbum->tracks[i].filename, 95);
      trackQueue[i][95] = '\0';
    } else {
      trackQueue[i][0] = '\0';
    }
  }
}

void cacheActiveAlbumArtwork(String path) {
  activeArtworkLoaded = false;
  File bmpFile = SD.open(path.c_str());

  if (!bmpFile) {
    Serial.printf("ARTWORK DETOUR: Direct open failed for %s. Scanning folder case-insensitively...\n", path.c_str());
    int lastSlash = path.lastIndexOf('/');
    if (lastSlash != -1) {
      String dirPath = path.substring(0, lastSlash + 1);
      File dir = SD.open(dirPath.c_str());
      if (dir) {
        while (true) {
          File entry = dir.openNextFile();
          if (!entry) break;
          if (!entry.isDirectory()) {
            String testName = String(entry.name());
            String upperTest = testName;
            upperTest.toUpperCase();

            if (upperTest == "FOLDER.BMP" || upperTest.endsWith(".BMP")) {
              String realPath = dirPath + testName;
              entry.close();
              bmpFile = SD.open(realPath.c_str());
              break;
            }
          }
          entry.close();
        }
        dir.close();
      }
    }
  }

  if (!bmpFile) {
    Serial.printf("ARTWORK CRITICAL ERROR: Image asset not found at path: %s\n", path.c_str());
    return;
  }

  bmpFile.seek(10);
  uint32_t dataOffset = 0;
  bmpFile.read((uint8_t*)&dataOffset, 4);

  int contentRowWidthBytes = 200 * 2;
  int fileStrideBytes = 400;

  for (int y = 0; y < 200; y++) {
    bmpFile.seek(dataOffset + ((uint32_t)y * fileStrideBytes));
    bmpFile.read((uint8_t*)&activeArtworkCache[(199 - y) * 200], contentRowWidthBytes);
  }

  bmpFile.close();
  activeArtworkLoaded = true;
  Serial.println("ARTWORK SUCCESS: Cache filled and oriented right-side up.");
}
