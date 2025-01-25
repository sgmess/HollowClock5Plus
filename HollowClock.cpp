#include "HollowClock.h"
#include "MotorControl.h"
#include "PreferencesManager.h"
#include "SoundPlayer.h"
#include "config.h"
#include "esp_sntp.h"
#include <thread>
#include <time.h>

#if DEBUG_HOLLOW_CLOCK
#define TRACE(...) Serial.printf(__VA_ARGS__)
#define ERROR(...) Serial.printf(__VA_ARGS__)
#else
#define TRACE(...)
#define ERROR(...)
#endif

#if USE_DEEP_SLEEP_WAKEUP_FOR_CLOCK
RTC_DATA_ATTR uint32_t unresettable_var;
#endif

enum {
  CMD_START = 1,          // start movement
  CMD_STOP = 2,           // stop movement
  CMD_STEP = 3,           // move steps
  CMD_UPDATE_POSITION = 4 // set hands at specific position
};

HollowClock &HollowClock::getInstance() {
  static HollowClock instance;
  return instance;
}

void HollowClock::adjustClockPosition(int steps) {
  // Adjust current position
  if (clock_position != PreferencesManager::INVALID_CLOCK_POSITION) {
    clock_position = (steps > 0)
                         ? (clock_position + steps) % max_clock_position
                         : (clock_position + max_clock_position + steps) %
                               max_clock_position;
  }
}
// 10:35 - 0:27
int HollowClock::calculateTimeDiff(int local_clock_position, int current_time,
                                   bool &direction_forward) {
  int time_diff = 0;

  direction_forward = true;
  if (allow_backward_movement) {
    if (local_clock_position > current_time) {
      current_time += max_clock_position;
    }
    if ((current_time - local_clock_position) > max_clock_position / 2) {
      direction_forward = false;
      time_diff = abs(max_clock_position - current_time + local_clock_position);
    } else {
      time_diff = abs(current_time - local_clock_position);
    }
    TRACE("Time diff: %d %d (%d)-> time_diff: %s%d\n",
          current_time - (direction_forward ? max_clock_position : 0) -
              local_clock_position,
          current_time - local_clock_position, max_clock_position / 2,
          direction_forward ? "" : "-", time_diff);
  } else {
    time_diff = (current_time + max_clock_position - local_clock_position) %
                max_clock_position;
    TRACE("Time diff: %d %d\n", local_clock_position - current_time,
          (current_time - local_clock_position + max_clock_position) %
              max_clock_position);
  }
  return time_diff;
}

void HollowClock::playChime(int current_time) {
  static int last_played_chime = -1;
  uint32_t hours = current_time / (60 * steps_per_minute);
  uint32_t minutes = current_time / steps_per_minute;
  if (play_chime) {
    // Play chime
    if (last_played_chime != hours && (minutes % 60 == 0)) {
      last_played_chime = hours;
      SoundPlayer::getInstance().playChime();
    }
  }
}

void HollowClock::saveClockPosition(void) {
  PreferencesManager &pm = PreferencesManager::getInstance();
  pm.setClock(clock_position);
}

void HollowClock::setDirection(bool direction) { flip_rotation = direction; }

void HollowClock::setAllowBackwardMovement(bool allow) {
  allow_backward_movement = allow;
}

void HollowClock::threadFunction(void) {
  bool clock_moving = true;
  MotorControl &motor = MotorControl::getInstance();
  while (true) {
    struct tm timeinfo;

    // Check for any new command
    uint32_t value;
    if (getFromQueue(value)) {
      uint8_t command = getCommand(value);
      TRACE("Command received: %d\n", command);
      switch (command) {
      case CMD_START:
        clock_moving = true;
        break;
      case CMD_STOP:
        clock_moving = false;
        break;
      case CMD_STEP:
        int steps;
        getParams(value, steps);
        TRACE("CMD_STEP: %d\n", steps);
        if (steps > 0) {
          while (steps > 0) {
            int step = (steps > MAX_FAST_MOVMENT_STEPS) ? MAX_FAST_MOVMENT_STEPS
                                                        : steps;
            motor.rotate(step, delay_time, flip_rotation);
            adjustClockPosition(step);
            steps -= step;
            delay(10);
          }
        } else if (allow_backward_movement) {
          while (steps < 0) {
            int step = (steps < -MAX_FAST_MOVMENT_STEPS)
                           ? -MAX_FAST_MOVMENT_STEPS
                           : steps;
            motor.rotate(step, delay_time, flip_rotation);
            adjustClockPosition(step);
            steps -= step;
            delay(10);
          }
        }
        break;
      case CMD_UPDATE_POSITION:
        uint8_t hours, minutes;
        getParams(value, hours, minutes);
        clock_position = (hours * 60 + minutes) * steps_per_minute;
        saveClockPosition();
        break;
      }
      continue;
    }

    if (clock_moving) {
      if (!::getLocalTime(&timeinfo)) {
        TRACE("Failed to obtain time\n");
        delay(5000);
        continue;
      }

      int hour = timeinfo.tm_hour % 12;
      int minute = timeinfo.tm_min;
      int current_time = (hour * 60 + minute) * steps_per_minute;

      if (clock_position == PreferencesManager::INVALID_CLOCK_POSITION) {
        // We don't know the current position of the clock so just tick
        motor.rotate(steps_per_minute / 16, delay_time, flip_rotation);
        adjustClockPosition(steps_per_minute / 16);
        delay(60000 / 16);
      } else {
        int local_clock_position = (int)clock_position;
        if (current_time != local_clock_position) {
          bool direction_forward = true;
          int time_diff = calculateTimeDiff(local_clock_position, current_time,
                                            direction_forward);
          if (time_diff > steps_per_minute) {
            positioning = true;
            TRACE("Positioning: current time: %d, Clock position: %d - "
                  "%stime_diff(sec):%d\n",
                  current_time, local_clock_position,
                  direction_forward ? "" : "-",
                  time_diff * 60 / steps_per_minute);

            time_diff = (time_diff > MAX_FAST_MOVMENT_STEPS)
                            ? MAX_FAST_MOVMENT_STEPS
                            : time_diff;
            if (time_diff <= MAX_FAST_MOVMENT_STEPS) {
              saveClockPosition();
            }
            motor.rotate(direction_forward ? time_diff : -time_diff, -1,
                         flip_rotation); // move fast to the current position
            adjustClockPosition(direction_forward ? time_diff : -time_diff);
            positioning = false;
            delay(10);
            continue;
          } else {
            int32_t local_clock_position = clock_position;
            TRACE("Current position: %d, Clock position: %d - "
                  "time_diff(sec):%d\n",
                  current_time, local_clock_position,
                  time_diff * 60 / steps_per_minute);
            motor.rotate(time_diff, delay_time, flip_rotation);
            playChime(current_time);
            adjustClockPosition(time_diff);
          }
        }
      }
    }
    delay(1000);
  }
}

bool HollowClock::isCalibrated(void) {
  return clock_position != PreferencesManager::INVALID_CLOCK_POSITION;
}

bool HollowClock::isPositioning(void) { return positioning; }

void HollowClock::getClockPosition(uint8_t &hours, uint8_t &minutes) {
  int position = clock_position;
  hours = position / (60 * steps_per_minute);
  minutes = (position % (60 * steps_per_minute)) / steps_per_minute;
}
hclock_result_t HollowClock::moveStart(void) {
  int value = makeCommand(CMD_START, 0);
  return addToQueue(value) ? HCLOCK_OK : HCLOCK_ERROR;
}
hclock_result_t HollowClock::moveStop(void) {
  int value = makeCommand(CMD_STOP, 0);
  return addToQueue(value) ? HCLOCK_OK : HCLOCK_ERROR;
}

hclock_result_t HollowClock::moveSteps(int steps) {
  int value = makeCommand(CMD_STEP, steps);
  return addToQueue(value) ? HCLOCK_OK : HCLOCK_ERROR;
}

hclock_result_t HollowClock::updateClockPosition(uint8_t hours,
                                                 uint8_t minutes) {
  int value = makeCommand(CMD_UPDATE_POSITION, hours, minutes);
  return addToQueue(value) ? HCLOCK_OK : HCLOCK_ERROR;
}

String HollowClock::getLocalTime(void) {
  struct tm timeinfo;
  if (!::getLocalTime(&timeinfo)) {
    return "Failed to obtain time";
  }
  char time_str[6];
  strftime(time_str, sizeof(time_str), "%I:%M", &timeinfo);
  return String(time_str);
}

void HollowClock::setLastSyncedTime(String time) { last_synced_time = time; }

String HollowClock::getLastSyncedTime(void) {
  TRACE("Sync status:%d\n", sntp_get_sync_status());
  return last_synced_time;
}

String HollowClock::getHandsPosition(void) {
  uint8_t hours, minutes;
  getClockPosition(hours, minutes);
  char time_str[6];
  if (hours == 0) {
    hours = 12;
  }
  snprintf(time_str, sizeof(time_str), "%2d:%02d", hours, minutes);
  return String(time_str);
}

void HollowClock::start(void) {
  clockThread = std::thread(std::bind(&HollowClock::threadFunction, this));
  clockThread.detach();
  started = true;
}

// Function to add a command to the queue
bool HollowClock::addToQueue(uint32_t value) {
  TRACE("Adding command %X to queue\n", value);
  std::lock_guard<std::mutex> lock(threadMutex);
  bool result = false;

  if (started && (commandQueue.size() < QUEUE_SIZE)) {
    commandQueue.push(value);
    queueCondition.notify_one();
    result = true;
  }
  return result;
}

// Function to get a command from the queue non-blocking
bool HollowClock::getFromQueue(uint32_t &value) {
  std::lock_guard<std::mutex> lock(threadMutex);
  if (commandQueue.empty()) {
    return false; // Indicate that the queue is empty
  }
  value = commandQueue.front();
  TRACE("Getting command %X from queue\n", value);
  commandQueue.pop();
  return true; // Successfully retrieved a command
}

uint32_t HollowClock::makeCommand(uint8_t cmd, uint8_t val1, uint8_t val2) {
  return (cmd << 24) | (val1 << 16) | (val2 << 8);
}

uint32_t HollowClock::makeCommand(uint8_t cmd, int val) {
  int sign = (val < 0) ? 1 : 0;
  val = abs(val);
  return (cmd << 24) | (sign << 23) | (val & 0x7FFFFF);
}

uint8_t HollowClock::getCommand(uint32_t value) { return value >> 24; }

void HollowClock::getParams(uint32_t value, uint8_t &par1, uint8_t &par2) {
  par1 = (value >> 16) & 0xFF;
  par2 = (value >> 8) & 0xFF;
}

void HollowClock::getParams(uint32_t value, int &par) {
  par = (value & 0x7FFFFF) * ((value >> 23) & 0x1 ? -1 : 1);
}

HollowClock::HollowClock() : started(false), positioning(false) {
  PreferencesManager &pm = PreferencesManager::getInstance();

  flip_rotation = pm.getFlipRotation();
  allow_backward_movement = pm.getAllowBackward();
  play_chime = pm.getChime();
  steps_per_minute = pm.getStepsPerMinute();
  delay_time = pm.getDelayTime();
  clock_position = pm.getClockPosition();
  max_clock_position = 12 * 60 * steps_per_minute;
  last_synced_time = "never!";

// Test
#if USE_DEEP_SLEEP_WAKEUP_FOR_CLOCK
  TRACE("Unresettable var: %d\n", unresettable_var);
  unresettable_var++;
#endif
}
