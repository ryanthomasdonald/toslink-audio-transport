#ifndef LIBRARY_MENU_H
#define LIBRARY_MENU_H

#include <Arduino.h>
#include "DisplayUI.h" 

// --- SLIM RAM-BASED NAVIGATION ENGINE MAPS ---
#define MAX_TRACKS_PER_ALBUM 30
#define MAX_ALBUMS_PER_ARTIST 10
#define MAX_ARTISTS_TOTAL 30

struct TrackEntry {
  char* filename; // Points exactly to the dynamically allocated filename length string
};

struct AlbumEntry {
  char* name;     // Points exactly to the album name string
  int trackCount;
  TrackEntry tracks[MAX_TRACKS_PER_ALBUM];
};

struct ArtistEntry {
  char* name;     // Points exactly to the artist name string
  int albumCount;
  AlbumEntry albums[MAX_ALBUMS_PER_ARTIST];
};

// Global Master Index pointers (now takes negligible static RAM space)
extern ArtistEntry library[MAX_ARTISTS_TOTAL];
extern int libraryArtistCount;

// --- NAVIGATION STATE ---
enum MenuLevel {
  LEVEL_ARTISTS, 
  LEVEL_ALBUMS,  
  LEVEL_TRACKS   
};

extern MenuLevel currentMenuLevel;
extern int selectedArtistIndex; 
extern int selectedAlbumIndex;  

// --- FUNCTIONS ---
void buildLibraryIndex(); 
void drawMenuScreen();
void processMenuTouch();

#endif
