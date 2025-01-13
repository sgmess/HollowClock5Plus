#include "MotorControl.h"
#include "config.h"

#if DEBUG
#define TRACE(...) Serial.printf(__VA_ARGS__)
#define ERROR(...) Serial.printf(__VA_ARGS__)
#define DBG(x) x
#else
#define TRACE(...)
#define ERROR(...)
#define DBG(x)
#endif

MotorControl &MotorControl::getInstance() {
  static MotorControl instance;
  return instance;
}

// original function from shiura modified by me
void MotorControl::rotate(int steps, int delaytime, bool flip_rotation) {
  int stepIndex, phaseIndex;
  int delta = (steps > 0) ? 1 : 3;
  int dt;

  if (delaytime > 0) {
    dt = delaytime * 3;
  } else {
    delaytime = 2;
    dt = 2 * delaytime;
  }
  int idx = flip_rotation ? 1 : 0;

  int abs_steps = (steps > 0) ? steps : -steps;
  for (phaseIndex = 0; phaseIndex < abs_steps; phaseIndex++) {
    phase = (phase + delta) % 4;
    for (stepIndex = 0; stepIndex < 4; stepIndex++) {
      digitalWrite(ports[idx][stepIndex], seq[phase][stepIndex]);
    }

    delay(dt);
    if (dt > delaytime)
      dt--;
  }
  // power cut
  for (stepIndex = 0; stepIndex < 4; stepIndex++) {
    digitalWrite(ports[idx][stepIndex], LOW);
  }
}

void MotorControl::playSound(unsigned int freq, unsigned int time) {
  int i, j, idx = 0;

  if (freq == 0) {
    return;
  }

  unsigned int waves = time * freq / 1000;
  int delay_value = 1000000 / (2 * freq);
  int phases[2];

  waves = waves & 0xFFFFFFFE; // make it even

  phases[0] = (phase + 1) % 4;
  phases[1] = (phase) % 4;

  DBG(unsigned long startTime = millis());

  for (j = 0; j < waves; j++) {
    for (i = 0; i < 4; i++) {
      digitalWrite(ports[0][i], seq[phases[idx]][i]);
    }
    idx = (idx + 1) % 2;
    delayMicroseconds(delay_value);
  }

  // power cut
  for (i = 0; i < 4; i++) {
    digitalWrite(ports[0][i], LOW);
  }

  DBG(unsigned long endTime = millis());
  DBG(unsigned long timePassed = endTime - startTime);
  DBG(Serial.printf("Time passed: %d ms\n", timePassed));
}

MotorControl::MotorControl() {
  int motor_ports[4] = CONFIG_MOTOR_PORTS;

  ports[0][0] = motor_ports[0];
  ports[0][1] = motor_ports[1];
  ports[0][2] = motor_ports[2];
  ports[0][3] = motor_ports[3];
  ports[1][0] = motor_ports[3];
  ports[1][1] = motor_ports[2];
  ports[1][2] = motor_ports[1];
  ports[1][3] = motor_ports[0];

  // Set the ports to output
  for (int i = 0; i < 4; i++) {
    pinMode(motor_ports[i], OUTPUT);
    digitalWrite(motor_ports[i], LOW);
  }

  phase = 0;
}