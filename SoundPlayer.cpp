// filepath: /Users/poopi/Documents/Arduino/HollowClock5Plus/SoundPlayer.cpp
#include "SoundPlayer.h"
#include "MotorControl.h"
#include "config.h"
#include "pitches.h"

#if DEBUG
#define TRACE(...) Serial.printf(__VA_ARGS__)
#define ERROR(...) Serial.printf(__VA_ARGS__)
#else
#define TRACE(...)
#define ERROR(...)
#endif

// Musics taken from https://github.com/robsoncouto/arduino-songs

#define TUNE_LENGTH(x) (sizeof(x) / sizeof(x[0]) / 2)

static const int16_t nokia_ringtone[] = {
    NOTE_E5,  8, NOTE_D5, 8, NOTE_FS4, 4, NOTE_GS4, 4, NOTE_CS5, 8,
    NOTE_B4,  8, NOTE_D4, 4, NOTE_E4,  4, NOTE_B4,  8, NOTE_A4,  8,
    NOTE_CS4, 4, NOTE_E4, 4, NOTE_A4,  2,
};

static const int16_t imperial_march[] = {
    NOTE_A4,  -4, NOTE_A4, -4, NOTE_A4,  16,  NOTE_A4, 16, NOTE_A4,  16,
    NOTE_A4,  16, NOTE_F4, 8,  REST,     8,   NOTE_A4, -4, NOTE_A4,  -4,
    NOTE_A4,  16, NOTE_A4, 16, NOTE_A4,  16,  NOTE_A4, 16, NOTE_F4,  8,
    REST,     8,  NOTE_A4, 4,  NOTE_A4,  4,   NOTE_A4, 4,  NOTE_F4,  -8,
    NOTE_C5,  16,

    NOTE_A4,  4,  NOTE_F4, -8, NOTE_C5,  16,  NOTE_A4, 2, // 4
    NOTE_E5,  4,  NOTE_E5, 4,  NOTE_E5,  4,   NOTE_F5, -8, NOTE_C5,  16,
    NOTE_A4,  4,  NOTE_F4, -8, NOTE_C5,  16,  NOTE_A4, 2,

    NOTE_A5,  4,  NOTE_A4, -8, NOTE_A4,  16,  NOTE_A5, 4,  NOTE_GS5, -8,
    NOTE_G5,  16, // 7
    NOTE_DS5, 16, NOTE_D5, 16, NOTE_DS5, 8,   REST,    8,  NOTE_A4,  8,
    NOTE_DS5, 4,  NOTE_D5, -8, NOTE_CS5, 16,

    NOTE_C5,  16, NOTE_B4, 16, NOTE_C5,  16,  REST,    8,  NOTE_F4,  8,
    NOTE_GS4, 4,  NOTE_F4, -8, NOTE_A4,  -16, // 9
    NOTE_C5,  4,  NOTE_A4, -8, NOTE_C5,  16,  NOTE_E5, 2,

    NOTE_A5,  4,  NOTE_A4, -8, NOTE_A4,  16,  NOTE_A5, 4,  NOTE_GS5, -8,
    NOTE_G5,  16, // 7
    NOTE_DS5, 16, NOTE_D5, 16, NOTE_DS5, 8,   REST,    8,  NOTE_A4,  8,
    NOTE_DS5, 4,  NOTE_D5, -8, NOTE_CS5, 16,

    NOTE_C5,  16, NOTE_B4, 16, NOTE_C5,  16,  REST,    8,  NOTE_F4,  8,
    NOTE_GS4, 4,  NOTE_F4, -8, NOTE_A4,  -16, // 9
    NOTE_A4,  4,  NOTE_F4, -8, NOTE_C5,  16,  NOTE_A4, 2,
};

static const int16_t ode_to_joy[] = {
    NOTE_E4, 4, NOTE_E4, 4, NOTE_F4, 4,  NOTE_G4, 4, // 1
    NOTE_G4, 4, NOTE_F4, 4, NOTE_E4, 4,  NOTE_D4, 4, NOTE_C4, 4, NOTE_C4, 4,
    NOTE_D4, 4, NOTE_E4, 4, NOTE_E4, -4, NOTE_D4, 8, NOTE_D4, 2,

    NOTE_E4, 4, NOTE_E4, 4, NOTE_F4, 4,  NOTE_G4, 4, // 4
    NOTE_G4, 4, NOTE_F4, 4, NOTE_E4, 4,  NOTE_D4, 4, NOTE_C4, 4, NOTE_C4, 4,
    NOTE_D4, 4, NOTE_E4, 4, NOTE_D4, -4, NOTE_C4, 8, NOTE_C4, 2,

    NOTE_D4, 4, NOTE_D4, 4, NOTE_E4, 4,  NOTE_C4, 4, // 8
    NOTE_D4, 4, NOTE_E4, 8, NOTE_F4, 8,  NOTE_E4, 4, NOTE_C4, 4, NOTE_D4, 4,
    NOTE_E4, 8, NOTE_F4, 8, NOTE_E4, 4,  NOTE_D4, 4, NOTE_C4, 4, NOTE_D4, 4,
    NOTE_G3, 2,

    NOTE_E4, 4, NOTE_E4, 4, NOTE_F4, 4,  NOTE_G4, 4, // 12
    NOTE_G4, 4, NOTE_F4, 4, NOTE_E4, 4,  NOTE_D4, 4, NOTE_C4, 4, NOTE_C4, 4,
    NOTE_D4, 4, NOTE_E4, 4, NOTE_D4, -4, NOTE_C4, 8, NOTE_C4, 2};

static const int16_t chime[] = {NOTE_G4, 4, NOTE_B4, 4, NOTE_A4, 4, NOTE_D4, 4,
                                NOTE_G4, 4, NOTE_A4, 4, NOTE_B4, 4, NOTE_G4, 4};

static const int16_t beep[] = {NOTE_B4, 8, NOTE_D4, 4};

SoundPlayer::Music music_nokiaTune(nokia_ringtone, TUNE_LENGTH(nokia_ringtone),
                                   180, 4);
SoundPlayer::Music music_imperialMarch(imperial_march,
                                       TUNE_LENGTH(imperial_march), 120, 4);
SoundPlayer::Music music_odeToJoy(ode_to_joy, TUNE_LENGTH(ode_to_joy), 140, 4);
SoundPlayer::Music music_chime(chime, TUNE_LENGTH(chime), 180, 2);
SoundPlayer::Music music_beep(beep, TUNE_LENGTH(beep), 120, 4);

SoundPlayer &SoundPlayer::getInstance() {
  static SoundPlayer instance;
  return instance;
}

SoundPlayer::SoundPlayer() {
  // Initialize sound player hardware or library here
}

int SoundPlayer::calculateWholeNoteDuration(int tempo,
                                            int timeSignatureDenominator) {
  // Calculate the duration of a whole note in milliseconds
  // Formula: (60000 ms per minute * 4 beats per whole note) / (tempo in beats
  // per minute) Adjust for time signature: multiply by
  // (timeSignatureDenominator / 4) Simplifying: 60000 *
  // timeSignatureDenominator / tempo
  if (tempo) {
    return 60000 * timeSignatureDenominator / tempo;
  } else {
    return 1000;
  }
}

void SoundPlayer::playMusic(Music music, bool pause_after_note) {
  int wholenote = calculateWholeNoteDuration(music.bpm, music.tsd);
  int note_type = 0, notelen = 0;

  for (int note = 0; note < music.length * 2; note = note + 2) {
    note_type = music.notes[note + 1];
    if (note_type > 0) {
      notelen = (wholenote) / note_type;
    } else if (note_type < 0) {
      notelen = (wholenote) / abs(note_type);
      notelen *= 1.5;
    }

    MotorControl::getInstance().playSound(music.notes[note], notelen);
    if (pause_after_note) {
      delay(notelen);
    }
  }
}

void SoundPlayer::playMusic(soundplayer_music_t music) {
  // Play the selected music
  switch (music) {
  case MUSIC_NOKIA_RINGTONE:
    playMusic(music_nokiaTune);
    break;
  case MUSIC_IMPERIAL_MARCH:
    playMusic(music_imperialMarch);
    break;
  case MUSIC_ODE_TO_JOY:
    playMusic(music_odeToJoy);
    break;
  default:
    ERROR("Unknown music to play\n");
    break;
  }
}

void SoundPlayer::playChime() {
  // Play the chime sound
  playMusic(music_chime, false);
}

void SoundPlayer::playBeep() {
  // Play the beep sound
  playMusic(music_beep, false);
}
