#ifndef LIBRARY_COMMON_H
#define LIBRARY_COMMON_H

#include <Arduino.h>
#include "DisplayUI.h"

#define MAX_TRACKS_PER_ALBUM 30
#define MAX_ALBUMS_PER_ARTIST 20
#define MAX_ARTISTS 150

// 🚀 DYNAMIC DATA STRUCTURES: We store strings as lean heap pointers (char*)
// instead of massive static character arrays (char[64])
struct TrackEntry {
  char* filename;  // Dynamically allocated on boot
};

struct AlbumEntry {
  char* name;
  char* artworkFilename;
  TrackEntry tracks[MAX_TRACKS_PER_ALBUM];
  int trackCount;
};

struct ArtistEntry {
  char* name;
  AlbumEntry albums[MAX_ALBUMS_PER_ARTIST];
  int albumCount;
};

extern ArtistEntry* library;
extern int libraryArtistCount;

extern int selectedArtistIndex;
extern int selectedAlbumIndex;
extern int currentTrackIndex;
extern int totalTracks;
extern bool activeEngineIsA;

// Bounded global path caches
extern char currentAlbumAbsolutePath[256];
extern char trackQueue[MAX_TRACKS_PER_ALBUM][96];

// Master Function Pipeline Declarations
void initLibrarySystem();
void buildLibraryIndex();
void populateTrackQueue();
void cacheActiveAlbumArtwork(String path);

#endif
