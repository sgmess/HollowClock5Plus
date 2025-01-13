#ifndef _MOTOR_CONTROL_H_
#define _MOTOR_CONTROL_H_

#include <Arduino.h>

class MotorControl {

public:
  static MotorControl &getInstance();
  MotorControl(const MotorControl &) = delete;
  MotorControl &operator=(const MotorControl &) = delete;

  void rotate(int steps, int delaytime, bool flip_rotation);
  void playSound(unsigned int freq, unsigned int time);

private:
  MotorControl();
  ~MotorControl() = default;

  int phase = 0;
  int ports[2][4];

  // sequence of stepper motor control
  const int seq[4][4] = {{LOW, LOW, HIGH, LOW},
                         {LOW, LOW, LOW, HIGH},
                         {HIGH, LOW, LOW, LOW},
                         {LOW, HIGH, LOW, LOW}};

};

#endif