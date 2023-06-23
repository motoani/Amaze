/*
* various generic includes for this task
*/
#ifndef AMAZEGENERIC_H
#define AMAZEGENERIC_H

//#define RACER_DEBUG // a flag for debug statements
//#define RACER_DEBUG_2 // debug statement in button callback perhaps
#define SHOW_RATE // a flag to show frame rate via serial port
//#define SHOW_TIMES // a flag to show various time points

#include <SPI.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7789 driver chip

#define FALSE 0x00
#define TRUE 0x01

// How many buttons do we use? Comment out to set to TWO
#define FOUR_BUTTONS

// The starter screen is only 128px square so other sizes give an untidy start
// Setting anything much large than 170 square seems a bit much for a single core ESP32
// 16:9 is 227x128 looks wide but will run at >20fps on dual core
// 16:9 also 170 x 302 gives rough playback with frametime exceeding 110ms
// Large screens still have some tearing
#define VIEW_WIDTH  200  // Set for Volo's competition with the little display to 128px, 
#define VIEW_HEIGHT 150  // AT 170 for T-display S3 the update time can reach 73ms if texture fills the screen, dual core more like 21 fps <50 ms

#define FLOOR_HORIZON (3+ VIEW_HEIGHT/2) // start raycasting the floor a bit below horizontal as the rays that probe infinity are 'wasted'

// Global variables needed by other modules declared here for all to see
extern TFT_eSPI tft;
extern TFT_eSprite view;

#endif // AMAZEGENERIC_H