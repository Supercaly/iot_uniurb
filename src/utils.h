#ifndef UTILS_H
#define UTILS_H

#include "config.h"

#include <Arduino.h>

// Constant used to reboot the device without waiting.
#define BOARD_REBOOT_DELAY_NOW 0

/*
 * Macro to turn on a LED.
 */
#define LED_ON(led) \
  do { \
    pinMode(led, OUTPUT); \
    digitalWrite(led, HIGH); \
  } while (0)

/*
 * Macro to turn off a LED.
 */
#define LED_OFF(led) \
  do { \
    pinMode(led, OUTPUT); \
    digitalWrite(led, LOW); \
  } while (0)

/*
 * Perform a full reboot on this board causing
 * the setup method to be called again.
 *
 * This method is intended to be called in case something
 * crucial is going bad and can cause a state of boot-loop if
 * not done correctly.
 */
void reboot_board(int wait_ms = BOARD_REBOOT_DELAY_MS);

#endif  // UTILS_H
