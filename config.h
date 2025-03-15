#ifndef _CONFIG_H
#define _CONFIG_H

#include <Arduino.h>

#define DEBUG 0

#if DEBUG
#define DEBUG_HOLLOW_CLOCK 0
#define DEBUG_CLOCK_WEB_SERVER 0
#define DEBUG_SOUND 0
#define DEBUG_MOTOR 0
#else
#define DEBUG_HOLLOW_CLOCK 0
#define DEBUG_CLOCK_WEB_SERVER 0
#define DEBUG_SOUND 0
#define DEBUG_MOTOR 0
#endif

#define USE_DEEP_SLEEP_WAKEUP_FOR_CLOCK 0
#define MAX_FAST_MOVMENT_STEPS 1000

// Ports used for the stepper motor
#define CONFIG_MOTOR_PORTS {9, 8, 7, 6}
#define SERIAL_BAUD_RATE 115200
#define WEBSERVER_PORT 80
#define DNS_PORT 53
#define DEFAULT_NTP_SERVER "pool.ntp.org"
#define DEFAULT_TIMEZONE_LOCATION "Etc/GMT"
#define DEFAULT_TIMEZONE "GMT0"
#define DEFAULT_AP_NAME_PREFIX "HOLLOW5P-"
#define DEFAULT_LOCALHOST_NAME "Hollow5Plus"
#define DEFAULT_NTP_UPDATE (60 * 60 * 12)

#ifndef STRING_VERSION
#define STRING_VERSION "1.0.5-dirty"
#endif

#ifndef STRING_DATE
#define STRING_DATE __DATE__
#endif

#endif