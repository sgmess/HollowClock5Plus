// filepath: /Users/poopi/Documents/Arduino/HollowClock5Plus/SoundPlayer.h
#ifndef _SOUND_PLAYER_H
#define _SOUND_PLAYER_H

#include <Arduino.h>

typedef enum {
  MUSIC_NOKIA_RINGTONE = 1,
  MUSIC_IMPERIAL_MARCH = 2,
  MUSIC_ODE_TO_JOY = 3
} soundplayer_music_t;

class SoundPlayer {

public:
  class Music {
  public:
    const int16_t *notes;
    int length;
    int bpm;
    int tsd;

    Music(const int16_t *notesArray, int len, int beatsPerMinute,
          int time_signature_denominator)
        : notes(notesArray), length(len), bpm(beatsPerMinute),
          tsd(time_signature_denominator) {}
  };

  static SoundPlayer &getInstance();
  SoundPlayer(const SoundPlayer &) = delete;
  SoundPlayer &operator=(const SoundPlayer &) = delete;

  void playMusic(Music music, bool pause_after_note = true);
  void playMusic(soundplayer_music_t music);
  void playChime();
  void playBeep();

private:
  SoundPlayer();
  ~SoundPlayer() = default;

  int calculateWholeNoteDuration(int tempo, int timeSignatureDenominator);
};

#endif