#ifndef LIBRARY_COMMON_H
#define LIBRARY_COMMON_H

#include <Arduino.h>
#include "DisplayUI.h"

#define MAX_ARTISTS_TOTAL 180
#define MAX_ALBUMS_PER_ARTIST 20
#define MAX_TRACKS_PER_ALBUM 35

struct TrackEntry {
  char* filename;
};

struct AlbumEntry {
  char* name;
  int trackCount;
  TrackEntry* tracks;
  char* artworkFilename;
};

struct ArtistEntry {
  char* name;
  int albumCount;
  AlbumEntry* albums;
};

enum UIState {
  STATE_PLAYER,
  STATE_MENU
};

enum MenuLevel {
  LEVEL_ARTISTS,
  LEVEL_ALBUMS,
  LEVEL_TRACKS
};

extern UIState currentUIState;
extern MenuLevel currentMenuLevel;
extern ArtistEntry library[MAX_ARTISTS_TOTAL];
extern int libraryArtistCount;
extern int selectedArtistIndex;
extern int selectedAlbumIndex;
extern int menuScrollOffset;

// 🚀 SCALED TO 200x200 FOR THE GREATER SCREEN FOOTPRINT
extern uint16_t activeArtworkCache[200 * 200];
extern bool activeArtworkLoaded;

void buildLibraryIndex();
void cacheActiveAlbumArtwork(String path);
void drawMenuSideButton(int x, int y, int w, int h, const char* label, uint16_t color);
void drawMenuScreen();
void processMenuTouch();
void drawArtistView();
void processArtistViewTouch();
void drawAlbumView();
void processAlbumViewTouch();
void drawTrackView();
void processTrackViewTouch();

#endif
