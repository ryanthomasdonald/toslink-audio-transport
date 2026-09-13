#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#define PATH_BUFFER_SIZE 128

#include <Arduino.h>
#include <Audio.h>

// 🛠️ FIX: Allocate true static array memory space rather than constant literal pointers
extern char currentArtistFolder[PATH_BUFFER_SIZE];
extern char currentAlbumFolder[PATH_BUFFER_SIZE];

extern char trackQueue[30][64];
extern int totalTracks;
extern int currentTrackIndex;
extern bool isMediaPlaying;
extern bool isMediaPaused;
extern bool activeEngineIsA;
extern bool nextTrackPreLaunched;

extern AudioPlaySdWav playWav1;
extern AudioPlaySdWav playWav2;

void initAudioSystem();
void scanCurrentAlbumFolder();
void updateAudioEngine();
void playFreshAlbumStart();
void preLoadNextTrack();

#endif
