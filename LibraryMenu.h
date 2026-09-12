#ifndef LIBRARY_MENU_H
#define LIBRARY_MENU_H

#include <Arduino.h>
#include "DisplayUI.h" 

#define MAX_TRACKS_PER_ALBUM 30
#define MAX_ALBUMS_PER_ARTIST 10
#define MAX_ARTISTS_TOTAL 30

struct TrackEntry {
  char* filename; 
};

struct AlbumEntry {
  char* name;     
  int trackCount;
  TrackEntry tracks[MAX_TRACKS_PER_ALBUM];
  char* artworkFilename; 
};

struct ArtistEntry {
  char* name;     
  int albumCount;
  AlbumEntry albums[MAX_ALBUMS_PER_ARTIST];
};

// Global Master Index pointers
extern ArtistEntry library[MAX_ARTISTS_TOTAL];
extern int libraryArtistCount;

// Expose the dynamic single cache details bounded exactly
extern uint16_t activeArtworkCache[160 * 160];
extern bool activeArtworkLoaded;

// --- NAVIGATION STATE ---
enum MenuLevel {
  LEVEL_ARTISTS, 
  LEVEL_ALBUMS,  
  LEVEL_TRACKS   
};

enum UIState {
  STATE_PLAYER,
  STATE_MENU
};

extern UIState currentUIState;
extern MenuLevel currentMenuLevel;
extern int selectedArtistIndex; 
extern int selectedAlbumIndex;  
extern int menuScrollOffset;

// --- FUNCTIONS ---
void scanRootForArtists();
void scanArtistForAlbums(String artist);
void scanAlbumForTracks(String artist, String album);
void cacheActiveAlbumArtwork(String path);
void drawMenuScreen();
void processMenuTouch();

#endif
