/*
* Definitions for RTOS-driven game board debounce
* April 2023
*/
#ifndef GAMEBOARDBUTTONS_H
#define GAMEBOARDBUTTONS_H

// As this is a .ino file the Arduino.h and frertos.h are automatically included
#include "timers.h"

#include"amaze_generic.h"
// Digital pin definitions for Volo's Game Board for ESP32 T-Display S3
// Note that none of these seem to be be RTC wake-up pins
// For the 2023 challenge we can only use two but others might have debug functions
#define CONTROL_PIN_COUNT 6 // Size of an array
#define CONTROL_LEFT 43     // The Arduino/ESP32 pin identifiers
#define CONTROL_UP 44
#define CONTROL_DOWN 18
#define CONTROL_RIGHT 17
#define CONTROL_A 21
#define CONTROL_B 16

#define BIT_CONTROL_LEFT  1 // THe bit that the pressed button will be present in
#define BIT_CONTROL_UP    2 // Note that the bit index also represents the postion in the array
#define BIT_CONTROL_DOWN  4
#define BIT_CONTROL_RIGHT 8
#define BIT_CONTROL_A     16
#define BIT_CONTROL_B     32

// Definitions for the queued debounce functions

  // Various RTOS-related variables
  TimerHandle_t Debounce_Reload_Timer;  // A software timer to debounce the controller keys
  BaseType_t DRT_Started;
  QueueHandle_t Controller_key_queue; // Button status will be stuffed into here

  // Variables and constants
  uint8_t control_pins[CONTROL_PIN_COUNT]={CONTROL_LEFT,CONTROL_UP,CONTROL_DOWN,CONTROL_RIGHT,CONTROL_A,CONTROL_B}; // This is the crux of the functions

  // Function prototypes
  uint8_t key_debounce(); // This isn't driven by RTOS
  void key_debounce_init(); // Sets things up for RTOS
  void key_debounce_callback(TimerHandle_t xTimer); // RTOS call back that does most of the work
  uint8_t key_debounce_check_queue(); // See if there is anything on the queue and return a bit-mapped summary of the buttons
  void key_wait(); // Halts in a loop until UP-DOWN_UP cycle done and it does randomisation too
#endif // GAMEBOARDBUTTONS_H