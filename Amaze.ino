/*
* 2023-04-26 Made core outline including button manager as tested in keyboard sketch
* debounce as a set of functions
* 2023-05-07
* Raycast routine adapted from https://lodev.org/cgtutor/raycasting.html for ESP32
* Depth cue shading is rapid and effective
* Camera FOV and magnification adjusted to give a fuller screen
* Being laid out for 128x128 but using additional screen area for mapping design etc
* Four button control on GAMER board but will need reducing for the competition...
* 2023-05-11
* Messing with int/float maths on color to be simpler, no need for a class
* Double to float as ESP32S3 FPU doesn't do doubles - saves only 2 to 3ms per frame though
* Tried and removed some lookup table pre-calculations that didn't speedup raycasting
* 2023-05-14
* Custom font and startup screen organised with non-RTOS buttons
* I wonder if this should become used throughout?
* 2023-05-17
* The skyline basically works and follows the FOV after a fashion, not tested deeply
* 2023-05-18
* Snow added to skyline and option for two buttons of control built in
* A basic detection of hits has been added too to begin end of the game actions
* 2023-05-18
* Button system put in place to compile in two or four buttons dependent on hardware
* 2023-15-20
* A simple end of play and portal system to move between levels is included
* Just enough to submit to Volo's challenge
* 2023-05-22
* Work on the floor to remove an error at horizon and save texturing where the walls are
* This may save a millisecond or so per loop
* 2023-05-31
* Working on allowing translucent walls which could act as windows or maybe even sprite-like
* It is working but adds more overhead than expected even when there isn't a transparent block in FOV
* 2023-06-01
* New textures reveal that overlaid transparency doesn't work well - more of an issue with solid walls?
* Fence-style textures show through both sides which looks odd
* 2023-06-03
* Vertical scale factor removed and vertical clipping to avoid sudden distortions at close range
* 2023-06-05
* Work on transparency with 'set to draw' by default and be cleared works OK
* NS/EW flag used to skip one side of drawing, only used for transparent texture which are most likely to be fences or windows
* Slows frame time to 40-60ms looking through two textures which isn't too shabby
* 2023-06-06
* Solid colour palette added to texture.h rather than code it
* 2023-06-07
* Setting up full 16 bit use in the world map
* 2023-07-13
* Started on dual core fork whuch will use RTOS to bring second core into play
* I am concerned that task switching will consume more time that it adds especially
* as there'll need to be more functions with all of their overhead
* Some constants moved globally and screenWidth from Lode's code eliminated
// 128x128 default timing measured here:
// On entry to maze - varied view - 24ms
// Forwards to stop - full texture - 40ms
// Floorcasting 1 to 6ms depending on openness as it doesn't plot walls
// Sky background is under 1ms, which implies there's little point in trying to do it on second core
* 2023-06-13
* Pulling blocks into functions where they are once per frame as the overhead won't be much
* and we need a better structure to use RTOS
* RTOS running although WDT is triggered - so blocked them!
* 2023-06-14
* Event flags used to start sky and floor redraw tasks and await their completion before walls
* Good description of FPU and tasks https://docs.espressif.com/projects/esp-idf/en/v5.0.2/esp32c3/api-reference/system/freertos_idf.html#symmetric-multiprocessing
* So important to minimise context switches involving FPU, none needed here!
* 2023-06-16
* Dual core set roughly arranged, some sample timings with 170px square
* ===RTOS with 1 wall task
* Level 1 entry 23fps, to wall  15fps, to white block 39fps
* ===RTOS with 2 wall tasks
*               25fps           21fps                 41fps     
* ===RTOS WITH ODD AND EVEN COLUMN RENDERING
*               30fps           21fps                 42fps - note first view is asymmetric so this spreads load better
* 2023-06-23
* Title screen bilinear transformation now done from and to 565 TFT image so doesn't need big buffer
* This seems to have avoided crashes which although reported as WDT were probably memory leaks
* Note thta reporting heap size seemed to crash too
*****************************************************************************************************
* THINGS TO DO
* Make transparent textures 'thin'
* Optimise transparency routine if possible
***************************************************************************************************** 
*/

/* RAY CASTING IS BASED ON THIS ROUTINE AS BELOW */
/*
Copyright (c) 2004-2021, Lode Vandevenne

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#include <SPI.h>
#include <TFT_eSPI.h> // Graphics and font library for ST7789 driver chip
#include <timers.h>   // Can also be used for RTOS timers
#define LOAD_GFXFF // Although I feel it's already done in the library setup file



#include "amaze_generic.h" // Various global declarations and macros
#include "game_board_buttons.h" // Defines the buttons and prototype debounce
#include "maze.h" // The world defined as an array and some other features
#include "textures.h" // Various wall and floor textures
#include "sky.h" // Makes a sky and hill backgound
#include "maze_title.h" // An image for the title screen from an AI generator
#include "Akhenaton_LYLD40pt7b.h" // A chosen freefont
#include "end_game.h" // What to do at the end
#include "floorcaster.h" // Floorcasting taken into a function
#include "game_logic.h" // What to do when we change direction etc

#define SIDE_SCALE 0.8 // x side fading factor

// Global variables needed by other modules and defined here
TFT_eSPI tft= TFT_eSPI();
TFT_eSprite view = TFT_eSprite(&tft);
TFT_eSprite title_screen = TFT_eSprite(&tft);

// RTOS Events
static EventGroupHandle_t raycast_event_group;

#define SKY_READY     ( 1 << 0 )    // set when the background and sky have been rendered
#define FLOOR_READY   ( 1 << 1 )   // set when the raycast floor has been rendered
                                  // These can run in parallel as they affect distinct sections of view sprite
                                  // but both must be complete before rendering the walls
#define SKY_REDRAW    ( 1 << 2 )   // These bits set when the view sprite has been sent to TFT so we can think about redrawing things
#define FLOOR_REDRAW  ( 1 << 3 )   // and cleared once the process starts
#define WALL_OD_REDRAW ( 1 << 4 )  // Trigger odd and even sides independently
#define WALL_EV_REDRAW ( 1 << 5 )
#define WALL_OD_READY ( 1 << 6 )   // Indicate that odd and even wallcaster tasks have finished
#define WALL_EV_READY ( 1 << 7 )


  float posX = START_X, posY = START_Y;  //x and y start position
  float dirX = -1, dirY = 0; //initial direction vector, this is upwards on TFT
  float planeX = 0, planeY = 0.8; //zoom in via camera plane vector, default was 0.66

/* Global constants too complex for pre-processor but that we don't want to compute frequently*/
  const float one_over_ViewWidth=1.0/VIEW_WIDTH; // A FPU multiply can be done later, no obvious time saving
  const float aspect_correct=(float)VIEW_HEIGHT*((float)VIEW_WIDTH/(float)VIEW_HEIGHT); // Adjust the height scaling so the cells stay square

  const float depth_shade=1.0/(1.5*(float)mapWidth);// Ensures the most distant wall is still not black
  const float one_over_2pi = 1/(M_PI*2.f); //Make a reciprocal so can multiply later

  // Work out FOV across the background, need to manipulate this across VIEW_WIDTH
  // so each pixel is a fractional part of 2PI
  // This is constant as we may move, but not rescale the camra during play
  // It would have to be recalculated if we allowed zoom in/out
  const float FOV_pixel = ((1<<SKY_SIZE)/VIEW_WIDTH)*(2*atan(sqrt(planeX*planeX+planeY*planeY)/sqrt(dirX*dirX+dirY*dirY)))/(2.f*M_PI);

  unsigned long game_start_time,game_duration; // This will be used as the gameplay duration
  int maze_choice; // Which maze are we looking at?

  // RTOS wallcaster variables, for dual core
  const int ev_start=0; // ie the even columns 
  const int od_start=1; // and the odd ones

  // Various background variables
  uint8_t back_height[1<<SKY_SIZE]; // Stores how high the mountains are at a given x pixel
  uint8_t snow_height[1<<SKY_SIZE]; // Stores the local snowline or zero if no snow to be shown
  
  int newtime = 0; //time of current frame, Don't bother with floating point!
  int startraytime; // time that raycasting entered
    
  #if defined SHOW_RATE  
  int fps;  // Frames per second is easier to understand than a duration per frame which may not be holistic
  #endif


// Reset postion and planes for use between levels
void player_position_reset()
  {
    posX = START_X, posY = START_Y;  //x and y start position
    dirX = -1, dirY = 0; //initial direction vector, this is upwards on TFT
    planeX = 0, planeY = 0.8; //zoom in via camera plane vector
  }

// TFT to RGB, we could return a 32bit word but that would need unpacking
void TFT_to_RGB (uint16_t tft565,uint8_t* Red,uint8_t* Green,uint8_t* Blue, bool swap)
  {
    uint16_t temp_red, temp_green, temp_blue, temp_word; 

    if (swap) tft565 = (tft565>>8) | (tft565<<8); // swap bytes

    *Red=(uint8_t)((tft565 & 0xf800)>>8);
    *Green=(uint8_t)((tft565 & 0x07e0)>>3);
    *Blue=(uint8_t)((tft565 & 0x001f)<<3);
  }

// RGB  to 565 for TFT
uint16_t RGB_to_TFT(uint8_t Red, uint8_t Green, uint8_t Blue, bool swap)
  {
    uint16_t tft565;
    uint16_t temp_red,temp_green,temp_blue, temp_word;


    temp_red=(Red & 0xF8)<<8; // Mask off trailing bits and shift into position 
    temp_green=(Green & 0xFC)<<3;
    temp_blue=Blue>>3;

    tft565=temp_red | temp_green | temp_blue; // OR the RGB together to 565 format

    if (swap) tft565 = (tft565>>8) | (tft565<<8); // swap bytes

    return (tft565);
  }

// RGB scaled and adjusted to 565 for TFT
// Tried using fixed point but just made more trouble and ESP32S3 float is reputedly just as fast, at least for multiply
uint16_t RGB_scale_TFT(uint32_t RedGreenBlue,float scale)
  {
    uint32_t long_red, long_green, long_blue;
    uint16_t temp_red, temp_green, temp_blue;
    uint16_t tft565;

    // A previous version had RGB as 3 floats, but as they were set per row etc, little gain
    long_red=   (RedGreenBlue & 0x00FF0000)>>16;
    long_green= (RedGreenBlue & 0x0000FF00)>>8;
    long_blue=  (RedGreenBlue & 0x000000FF);

    temp_red= (uint16_t)(scale*(float)long_red);
    temp_green= (uint16_t)(scale*(float)long_green);
    temp_blue= (uint16_t)(scale*(float)long_blue);

    temp_red=(temp_red & 0xF8)<<8; // Mask off trailing bits and shift into position 
    temp_green=(temp_green & 0xFC)<<3;
    temp_blue=temp_blue>>3;

    tft565=temp_red | temp_green | temp_blue; // OR the RGB together to 565 format
    return (tft565);
  }

void setup() {
  // put your setup code here, to run once:

    { // Use Serial Monitor to see output, but with a short delay
      #if defined SHOW_RATE || defined SHOW_TIMES
      delay(200);
      Serial.begin(115200);
      Serial.println("\nEntering Amaze");
      #endif
    }
  // Initialise the debouce function, but doesn't appear to be properly active in setup() function
  key_debounce_init(); // A class was too complex and there'll never be more than one!
  
  // Initialise the TFT driver and show a startup
  tft.init();
  tft.fillScreen(TFT_BLACK); // clear the screen 
  tft.setRotation(1);
  tft.setSwapBytes(true); // seems to be required

  view.createSprite(VIEW_WIDTH,VIEW_HEIGHT); 
  //view.setSwapBytes(true);
  view.fillSprite(TFT_BLACK);// clear the view sprite frame buffer before we build the world

  // Compose the opening screen
  title_screen.createSprite(TITLE_WIDTH,TITLE_HEIGHT); // Make a sprite
  title_screen.setSwapBytes(true);

  title_screen.setFreeFont(&Akhenaton_LYLD40pt7b); // Pick a font - thsi size suits title of 128px
  title_screen.setTextColor(TFT_WHITE);
  title_screen.setTextDatum(TC_DATUM); // Top centre, so text appear a little below the centreline

  title_screen.pushImage(0,0,TITLE_WIDTH,TITLE_HEIGHT,maze_title); // Put the title image in it

  title_screen.drawString("Amaze",TITLE_WIDTH/2,TITLE_HEIGHT/2); // Write the game name into the middle of the TITLE so it can be scaled with the image

  // We'll resize the title screen if required, push to a sprite
  // write on some text
  // and finally push the sprite to the TFT
  if ((VIEW_WIDTH != TITLE_WIDTH)||(VIEW_HEIGHT != TITLE_HEIGHT))
    {
      // resize the title screen with its text between two sprite meory areas
      bilerp((uint16_t *)view.getPointer(), VIEW_WIDTH, VIEW_HEIGHT, (uint16_t *) title_screen.getPointer(), TITLE_WIDTH, TITLE_HEIGHT,true); // re-size the image
    }
  else
    {
      title_screen.pushToSprite(&view,0,0); // Copy title screen to view sprit unaltered
    }
  view.pushSprite(0,0); // Push the sprite to be seen as the opening screen
  
  maze_choice=0; // Start on first worldMap

  sky_build(); // Make the random hills and snow of the background

// The WDT gets triggered with the big tasks of raycasting - but not really sure why!
  disableCore0WDT();  // This is required or else the WDT triggers and the board resets
  //disableCore1WDT(); // This says it can't be done??

  // Setup the RTOS tasks that will drive the game
  raycast_event_group=xEventGroupCreate();
  if (!raycast_event_group) Serial.println("Raycast event group can't be created.");

  key_wait(); // Don't go until player presses

  
xTaskCreate(
                    show_sky,        // Task function
                    "Show sky",      // String with name of task
                    5000,            // Stack size in bytes
                    NULL,            // Parameter passed as input of the task
                    1,               // Priority of the task
                    NULL);           // Task handle

xTaskCreate(
                    floorcaster,     // Task function
                    "Floor caster",  // String with name of task
                    5000,            // Stack size in bytes
                    NULL,            // Parameter passed as input of the task
                    1,               // Priority of the task
                    NULL);           // Task handle

xTaskCreate(
                    game_loop,        // Task function
                    "Game loop",      // String with name of task
                    10000,            // Stack size in bytes
                    NULL,             // Parameter passed as input of the task
                    1,                // Priority of the task
                    NULL);            // Task handle

xTaskCreatePinnedToCore (
                    wall_caster,        // Task function
                    "Odd wall caster",      // String with name of task
                    10000,            // Stack size in bytes
                    (void*)&od_start,// Parameter passed as input of the task
                    1,                // Priority of the task
                    NULL,             // Task handle
                    0);            // Core

xTaskCreatePinnedToCore (
                    wall_caster,        // Task function - same as the above but different starting point
                    "Even wall caster",      // String with name of task
                    10000,            // Stack size in bytes
                    (void*)&ev_start,// Parameter passed as input of the task
                    1,                // Priority of the task
                    NULL,             // Task handle
                    1);            // Core

// Set some ready flags to start it running!
  xEventGroupSetBits(
                    raycast_event_group,
                    SKY_REDRAW | FLOOR_REDRAW);
                      
  game_start_time=millis(); // Record when we begin
#ifdef RACER_DEBUG
Serial.println("End of setup");
#endif
} // End of setup()

void loop(){
  delay(100);
  // Shouldn't ever get here...
  } // The traditional Arduino loop

void game_loop(void * parameter ){
for(;;) // Loop forever
{
#ifdef RACER_DEBUG
Serial.println("Start of loop");Serial.println("");
#endif

    startraytime=millis(); // note when we entered the beginning of raycasting algorithm

    //FLOOR CASTING
    //floorcaster();
 
    // SKY DISPLAY FROM STORED PEAK AND SNOW ARRAYS  
    //show_sky();
    // Wait until the bits say that the sky and floor are drawn
    xEventGroupWaitBits(
                       raycast_event_group,               // event group handle
                       SKY_READY | FLOOR_READY,           // bits to wait for
                       pdTRUE,                            // clear the bit once we've started
                       pdTRUE,                            //  AND for any of the defined bits
                       portMAX_DELAY );                   //  block forever

    
    #ifdef SHOW_TIMES
    int floortime=millis();
    Serial.print("Floor cast time: ");
    Serial.println(floortime-startraytime); // output the raycast time
    #endif

#ifdef RACER_DEBUG
Serial.println("End of floor casting");
#endif
   // END OF SKY DISPLAY
    #ifdef SHOW_TIMES
    int skytime=millis();
    Serial.print("Sky time: ");
    Serial.print(skytime-floortime); // output the raycast time
    Serial.print(" Floor and sky time: ");
    Serial.println(skytime-startraytime); // output the raycast time
    #endif

      #ifdef RACER_DEBUG
      Serial.println("End of sky so now walls...");
      #endif
  // Now it's pushed, signal to wall task that they can be made
      xEventGroupSetBits(
                      raycast_event_group,     // the handle
                      WALL_OD_REDRAW | WALL_EV_REDRAW);             // set a bit to request drawing on both columns

//    wall_caster();
    // Wait until the bits say that BOTH the left and right walls are drawn
    xEventGroupWaitBits(
                       raycast_event_group,               // event group handle
                       WALL_OD_READY | WALL_EV_READY,           // bits to wait for
                       pdTRUE,                            // clear the bit once we've started
                       pdTRUE,                            //  AND for any of the defined bits
                       portMAX_DELAY );                   //  block forever

// ESP32: draws text on screen
    #ifdef RACER_DEBUG
    Serial.print("DirX: ");
    Serial.print(dirX);
    Serial.print("DirY: ");
    Serial.println(dirY);
    #endif
 
 //int test=millis();
 // ESP32: redraws the screen buffer after everything has been placed
view.pushSprite(0,0);  // Display the view sprite on the TFT - this seems to take 14ms!!!
//Serial.println(millis()-test);

  // Now it's pushed, signal to tasks that redrawing can begin
      xEventGroupSetBits(
                      raycast_event_group,     // the handle
                      SKY_REDRAW | FLOOR_REDRAW);             // set a bit to request drawing

  game_logic(); // Decide what happens next
  
  #if defined SHOW_RATE  
  fps++; // Increment fps counter (it will be reset and shown by a 1 second timer)
  #endif
  } // End of forever loop
} // End of game_loop[ function]
