#include "AudioEngine.h"
#include "DisplayUI.h"
#include <SD.h>

// Standard mutable array instantiation matching our header specifications
char currentArtistFolder[PATH_BUFFER_SIZE] = "Kaddisfly/";
char currentAlbumFolder[PATH_BUFFER_SIZE] = "Set Sail the Prarie/";

// Instantiations matching our master header parameters
char trackQueue[30][96];
int totalTracks = 0;
int currentTrackIndex = 0;
bool isMediaPlaying = false;
bool isMediaPaused = false;
bool activeEngineIsA = true;
bool nextTrackPreLaunched = false;

AudioPlaySdWav playWav1;
AudioPlaySdWav playWav2;
AudioMixer4 audioMixerL;
AudioMixer4 audioMixerR;
AudioOutputSPDIF3 spdif1;

AudioConnection patchCord1(playWav1, 0, audioMixerL, 0);
AudioConnection patchCord2(playWav2, 0, audioMixerL, 1);
AudioConnection patchCord3(audioMixerL, 0, spdif1, 0);
AudioConnection patchCord4(playWav1, 1, audioMixerR, 0);
AudioConnection patchCord5(playWav2, 1, audioMixerR, 1);
AudioConnection patchCord6(audioMixerR, 0, spdif1, 1);

void initAudioSystem() {
  AudioMemory(48);
  audioMixerL.gain(0, 1.0);
  audioMixerL.gain(1, 0.0);
  audioMixerR.gain(0, 1.0);
  audioMixerR.gain(1, 0.0);
}

void scanCurrentAlbumFolder() {
  totalTracks = 0;
  currentTrackIndex = 0;
  String fullPath = String(currentArtistFolder) + String(currentAlbumFolder);
  File dir = SD.open(fullPath.c_str());
  if (!dir) {
    Serial.printf("Scanner Error: Cannot open folder directory: %s\n", fullPath.c_str());
    return;
  }
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break;
    if (!entry.isDirectory()) {
      String name = String(entry.name());
      if (name.endsWith(".wav") || name.endsWith(".WAV")) {
        if (totalTracks < 30) {
          name.toCharArray(trackQueue[totalTracks], 96);  // 🚀 Expanded buffer ceiling from 64 to 96
          totalTracks++;
        }
      }
    }
    entry.close();
  }
  dir.close();

  // Alphabetical sort to keep your audio tracks strictly sequential (01, 02, 03...)
  for (int i = 0; i < totalTracks - 1; i++) {
    for (int j = i + 1; j < totalTracks; j++) {
      if (strcmp(trackQueue[i], trackQueue[j]) > 0) {
        char temp[64];
        strcpy(temp, trackQueue[i]);
        strcpy(trackQueue[i], trackQueue[j]);
        strcpy(trackQueue[j], temp);
      }
    }
  }
  Serial.printf("Scanner Engine: Success. Loaded %d audio tracks from nested directory.\n", totalTracks);
}

void playFreshAlbumStart() {
  playWav1.stop();
  playWav2.stop();
  if (totalTracks == 0) return;
  activeEngineIsA = true;
  nextTrackPreLaunched = false;
  resetProgressTrackers();
  audioMixerL.gain(0, 1.0);
  audioMixerL.gain(1, 0.0);
  audioMixerR.gain(0, 1.0);
  audioMixerR.gain(1, 0.0);
  String fullPath = String(currentArtistFolder) + String(currentAlbumFolder) + trackQueue[currentTrackIndex];
  if (playWav1.play(fullPath.c_str())) {
    isMediaPlaying = true;
    isMediaPaused = false;

    // 🚀 FIX CONFIRMED: Forces vector twin-bars icon onto the deck on fresh boot/select
    updatePlayPauseButtonLabel("||", COLOR_RAMS_CARD);
    updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);
  } else {
    updateTrackWindow(currentTrackIndex + 1, "FILE NOT FOUND");
  }
}

void preLoadNextTrack() {
  int nextTrackIndex = currentTrackIndex + 1;
  if (nextTrackIndex >= totalTracks) return;
  String fullPath = String(currentArtistFolder) + String(currentAlbumFolder) + trackQueue[nextTrackIndex];
  if (activeEngineIsA) {
    if (playWav2.play(fullPath.c_str())) {
      delay(5);
      playWav2.togglePlayPause();
    }
  } else {
    if (playWav1.play(fullPath.c_str())) {
      delay(5);
      playWav1.togglePlayPause();
    }
  }
}

void updateAudioEngine() {
  if (!isMediaPlaying) return;
  int nextTrackIndex = currentTrackIndex + 1;
  if (activeEngineIsA) {
    if (!nextTrackPreLaunched && nextTrackIndex < totalTracks) {
      preLoadNextTrack();
      nextTrackPreLaunched = true;
    }
    if (!playWav1.isPlaying()) {
      if (nextTrackIndex < totalTracks) {
        AudioNoInterrupts();
        audioMixerL.gain(0, 0.0);
        audioMixerL.gain(1, 1.0);
        audioMixerR.gain(0, 0.0);
        audioMixerR.gain(1, 1.0);
        AudioInterrupts();
        playWav2.togglePlayPause();
        currentTrackIndex++;
        activeEngineIsA = false;
        nextTrackPreLaunched = false;
        updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);
      } else {
        isMediaPlaying = false;
        // 🚀 FIX CONFIRMED: Returns button deck cleanly to triangle vector icon
        updatePlayPauseButtonLabel(">", COLOR_RAMS_CARD);
        updateTrackWindow(currentTrackIndex + 1, "ALBUM FINISHED");
      }
    }
  } else {
    if (!nextTrackPreLaunched && nextTrackIndex < totalTracks) {
      preLoadNextTrack();
      nextTrackPreLaunched = true;
    }
    if (!playWav2.isPlaying()) {
      if (nextTrackIndex < totalTracks) {
        AudioNoInterrupts();
        audioMixerL.gain(0, 1.0);
        audioMixerL.gain(1, 0.0);
        audioMixerR.gain(0, 1.0);
        audioMixerR.gain(1, 0.0);
        AudioInterrupts();
        playWav1.togglePlayPause();
        currentTrackIndex++;
        activeEngineIsA = true;
        nextTrackPreLaunched = false;
        updateTrackWindow(currentTrackIndex + 1, trackQueue[currentTrackIndex]);
      } else {
        isMediaPlaying = false;
        // 🚀 FIX CONFIRMED: Returns button deck cleanly to triangle vector icon
        updatePlayPauseButtonLabel(">", COLOR_RAMS_CARD);
        updateTrackWindow(currentTrackIndex + 1, "ALBUM FINISHED");
      }
    }
  }
}
