/*
* various generic includes for this task
*/
#ifndef AMAZEGENERIC_H
#define AMAZEGENERIC_H

//#define RACER_DEBUG // a flag for debug statementsbutton callback
//#define RACER_DEBUG_2 // debug statement in 
#define SHOW_RATE // a flag to show frame rate via serial port

#include <SPI.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7789 driver chip

#define FALSE 0x00
#define TRUE 0x01

// How many buttons do we use? Comment out to set to TWO
#define FOUR_BUTTONS

// The starter screen is only 128px square so other sizes give an untidy start
// Setting anything much larger than 170 square seems a bit much for a single core ESP32
// 16:9 is 227x128 looks wide and frame time 60 to 70+ ms with multiple transparencies
#define VIEW_WIDTH  128  // Set for Volo's competition with the little display to 128px
#define VIEW_HEIGHT 128 // AT 170 for T-display S3 the update time can reach 73ms if texture fills the screen

#define FLOOR_HORIZON (3+ VIEW_HEIGHT/2) // start raycasting the floor a bit below horizontal as the rays that probe infinity are 'wasted'

// Global variables needed by other modules declared here for all to see
extern TFT_eSPI tft;
extern TFT_eSprite view;

#endif // AMAZEGENERIC_H