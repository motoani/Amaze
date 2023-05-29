/*
* 2023-04-26 Made core outline including button manager as tested in keyboard sketch
* debounce as a set of functions
* 2023-05-07
* Raycast routine adapted from https://lodev.org/cgtutor/raycasting.html for ESP32
* Depth cue shading is rapid and effective
* Camera FOV and magnification adjeusted to give a fuller screen
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


#define VIEW_SCALE 1.1 // Adjust the size shown, a Y axis multiplier
#define SIDE_SCALE 0.8 // x side fading factor

// Global variables needed by other modules and defined here
TFT_eSPI tft= TFT_eSPI();
TFT_eSprite view = TFT_eSprite(&tft);
#ifdef SHOW_MAZE
TFT_eSprite mapview = TFT_eSprite(&tft);
TFT_eSprite player =  TFT_eSprite(&tft);
#endif


  float posX = START_X, posY = START_Y;  //x and y start position
  float dirX = -1, dirY = 0; //initial direction vector, this is upwards on TFT
  //float planeX = 0, planeY = 0.66; //the 2d raycaster version of camera plane
  float planeX = 0, planeY = 0.8; //zoom in via camera plane vector

  const float one_over_screenWidth=1.0/VIEW_WIDTH; // A FPU multiply can be done later, no obvious time saving
  //const float one_over_w=1.0/w; // Another FPU divide avoided

  unsigned long game_start_time,game_duration; // This will be used as the gameplay duration
  int maze_choice; // Which maze are we looking at?

  // Various background variables
  uint8_t back_height[1<<SKY_SIZE]; // Stores how high the mountains are at a given x pixel
  uint8_t snow_height[1<<SKY_SIZE]; // Stores the local snowline or zero if no snow to be shown

  // Work out FOV across the background, need to manipulate this across VIEW_WIDTH
  // so each pixel is a fractional part of 2PI
  const float FOV_pixel = ((1<<SKY_SIZE)/VIEW_WIDTH)*(2*atan(sqrt(planeX*planeX+planeY*planeY)/sqrt(dirX*dirX+dirY*dirY)))/(2.f*M_PI);
  
  // double time = 0; //time of current frame
  // double oldTime = 0; //time of previous frame
  int newtime = 0; //time of current frame, Don't bother with floating point!
  //int oldtime = 0; //time of previous frame
  int startraytime; // time that raycasting entered

// Reset postion and planes for use between levels
void player_position_reset()
  {
    posX = START_X, posY = START_Y;  //x and y start position
    dirX = -1, dirY = 0; //initial direction vector, this is upwards on TFT
    planeX = 0, planeY = 0.8; //zoom in via camera plane vector
  }

// RGB scaled and adjusted to 565 for TFT
// Tried using fixed point but just made more trouble and ESP32S3 float is reputedly just as fast
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

    tft565=temp_red | temp_blue | temp_green; // OR the RGB together to 565 format
    return (tft565);
  }

void setup() {
  // put your setup code here, to run once:

  // Initialise the debouce function, but doesn't appear to be properly active in setup() function
  key_debounce_init(); // A class was too complex and there'll never be more than one!
   
{ // Initialise the TFT driver and show a startup
  tft.init();
  tft.fillScreen(TFT_BLACK); // clear the screen 
  tft.setRotation(1);
  tft.setSwapBytes(true); // Missing both, or just in 'frame', swaps gives the correct colour rendering
  tft.pushImage(0,0,128,128,maze_title); // Show the startup image for the game

  tft.setFreeFont(&Akhenaton_LYLD40pt7b);
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(TC_DATUM);

    { // Use Serial Monitor to see output, but with a short delay
      #ifdef SHOW_RATE
      delay(200);
      Serial.begin(115200);
      #endif
    }

  tft.drawString("Amaze",VIEW_WIDTH/2,VIEW_HEIGHT/2); // Write the game name
  }

  view.createSprite(VIEW_WIDTH,VIEW_HEIGHT);
  view.fillSprite(TFT_BLACK);// clear the view sprite frame buffer before we build the world
  #ifdef SHOW_MAZE
  mapview.createSprite(VIEW_WIDTH,VIEW_HEIGHT);
  #endif

  maze_choice=0; // Start on first worldMap

#ifdef SHOW_MAZE
  maze_init();
  draw_maze();
#endif

  sky_build();

  key_wait(); // Don't go until player presses

  game_start_time=millis(); // Record when we begin
} // End of setup()

void loop(){
      uint8_t key_pressed; // from the button queue
      const float depth_shade=1.0/(1.5*(float)mapWidth);// Ensures the most distant wall is still not black
      float face_shade=1.0; // this will adjust brightness to give a simple shade of perpendicular surfaces
      
      const float one_over_2pi = 1/(M_PI*2.f); //Make a reciprocal so can multiply later

    startraytime=millis(); // note when we entered the beginning of raycasting algorithm

    //FLOOR CASTING
  
    for(int y = FLOOR_HORIZON; y < VIEW_HEIGHT; y++)  // Added h/2 to miss the ceiling 3+ to avoid the horizontal rays
    {
      // rayDir for leftmost ray (x = 0) and rightmost ray (x = width)
      float rayDirX0 = dirX - planeX;
      float rayDirY0 = dirY - planeY;
      float rayDirX1 = dirX + planeX;
      float rayDirY1 = dirY + planeY;
/*
      // Current y position compared to the center of the screen (the horizon)
      int p = y - screenHeight / 2;

      // Vertical position of the camera.
      float posZ = 0.5 * screenHeight;

      // Horizontal distance from the camera to the floor for the current row.
      // 0.5 is the z position exactly in the middle between floor and ceiling.
      float rowDistance = posZ / p;
*/
      // The 3 calculations above can be substituted with one equation:
      float rowDistance=(float)(VIEW_HEIGHT/2)/(float)(y-VIEW_HEIGHT/2); 
      // This gives a divide by zero at the absolute horizon
      // Thus the ray is almost horizontal and will escape from the world
      // Simple capping is not enough as that shows the wrong pixels.
      // This doesn't draw the horizon and leaves a sapce below the sky. That's FLOOR_HORIZON
      // A lookup table of this gave no apparent speed increase

      // calculate the real world step vector we have to add for each x (parallel to camera plane)
      // adding step by step avoids multiplications with a weight in the inner loop
      //float floorStepX = rowDistance * (rayDirX1 - rayDirX0) / screenWidth;
      float floorStepX = rowDistance * (rayDirX1 - rayDirX0) * one_over_screenWidth; // Swapped a divide for a multiply by the reciprocal
      float floorStepY = rowDistance * (rayDirY1 - rayDirY0) * one_over_screenWidth;

      // real world coordinates of the leftmost column. This will be updated as we step to the right.
      float floorX = posX + rowDistance * rayDirX0;
      float floorY = posY + rowDistance * rayDirY0;

      for(int x = 0; x < VIEW_WIDTH; ++x)
      {
        // the cell coord is simply got from the integer parts of floorX and floorY
        int cellX = (int)(floorX);
        int cellY = (int)(floorY);

        int texNum = worldMap[maze_choice][cellX][cellY]; // Find out what is in the cell
        unsigned long color;
        if (!(texNum & MAP_WALL_FLAG)) // It's a wall, not a floor, so skip mapping into a tile and sending pixels
        // This does mean that some sections of the floor are not drawn but they should be covered with wall! 
        // THis should also save time especially when they are textured
        {
        if (texNum & MAP_TEXTURE_FLAG)
          {
            // get the texture coordinate from the fractional part
            int tx = (int)(texWidth * (floorX - cellX)) & (texWidth - 1);
            int ty = (int)(texHeight * (floorY - cellY)) & (texHeight - 1);

            // floor mapping - This adds around 5ms to the frame duration
            color = texture[texNum & MAP_CHOICE_MASK][texWidth * ty + tx];
        
          } // End of texture mapping
        else
          { // The cell isn't textured, but due to projection we still have to do pixel by pixel
            switch(MAP_CHOICE_MASK & texNum)
            {
              case 1:  color=0x00e01010;    break; //red
              case 2:  color=0x0010e010;  break; //green
              case 3:  color=0x000000ff;   break; //blue
              case 4:  color=0x00ffffff;  break; //white
              case 5:  color=0x00ffff00;  break; //yellow
              default: color=0x00808080; break; //grey
            }
          } // End of plain colour fill
        // Plot the colored floor pixel and also do depth cue shading  
        view.drawPixel(x,y,RGB_scale_TFT(color,1.0-(rowDistance*depth_shade)));
        } // End of deciding if it really is floor

        floorX += floorStepX;
        floorY += floorStepY;
      }
    }

#ifdef RACER_DEBUG
int skystart=micros();
#endif
// Use the sky/mountain array to build the top half of the display
// This takes between 500 and 900 microseconds dependent on mountain and snow lengths at 170 pixels square
// Start by colouring the 'sky'
  view.fillRect(0, 0, VIEW_WIDTH, VIEW_HEIGHT/2, SKY_BLUE); // This will leave a slight gap at horizon

// SKY DISPLAY FROM STORED PEAK AND SNOW ARRAYS  
// Now use the array to make the mountain range
// Use the camera plane vector to choose starting point in the array
// Whilst the view is centred perpendicular to the plane, and that would be the middle of the FOV
// as this is just a constant rotation value it can be ignored!

  // Work out which way we are facing
    float FOV_angle=atan2(dirY,dirX);
    if (dirY<0) FOV_angle=M_PI*2.f + FOV_angle; // Adjust so always zero to 2*PI]
    
    FOV_angle=(M_PI*2.f)-FOV_angle; // But switch direction!
    int start_index=(int)((1<<SKY_SIZE)*(FOV_angle*one_over_2pi)); // Swapped a divide for a multiply by the reciprocal although not a time critical spot

#ifdef RACER_DEBUG
  Serial.print ("Angle ");
  Serial.print(FOV_angle); // Where are we facing?
  Serial.print(" ");
  Serial.println(start_index);
#endif

  for (int x=0;x<VIEW_WIDTH;++x)
    { // Display the visible skyline of mountains in it
      int back_index=SKY_MASK & (int)((FOV_pixel*(float)x)+start_index); // Corrected to show correct FOV
      //int back_index=SKY_MASK & (int)(x+start_index); // Simpler but better effect?
      //Serial.println(FOV_pixel);
      view.drawFastVLine(x, VIEW_HEIGHT/2-back_height[back_index], back_height[back_index], SKY_BROWN);
      
      // Check and maybe draw snow, from the mountain peak dowm which is increasing pixel Y position 
      if (snow_height) view.drawFastVLine(x, VIEW_HEIGHT/2-back_height[back_index], snow_height[back_index], SKY_SNOW);
    }
    #ifdef RACER_DEBUG
    int skyend=micros();
    Serial.print("sky ");Serial.println(skyend-skystart);
    #endif
// END OF SKY DISPLAY

    // WALL CASTING WITH SHADING AND OPTIONAL TEXTURES
    for(int x = 0; x < VIEW_WIDTH; x++)
    {
      #ifdef RACER_DEBUG
      Serial.print("Raycast x: ");
      Serial.println(x);
      #endif

      //calculate ray position and direction
      float cameraX = 2 * x * one_over_screenWidth - 1; //x-coordinate in camera space
      float rayDirX = dirX + planeX * cameraX; // A lo0kup table was actually slower! 
      float rayDirY = dirY + planeY * cameraX;
      // which box of the map we're in
      int mapX = int(posX);
      int mapY = int(posY);

      //length of ray from current position to next x or y-side
      float sideDistX;
      float sideDistY;

      //length of ray from one x or y-side to next x or y-side
      //these are derived as:
      //deltaDistX = sqrt(1 + (rayDirY * rayDirY) / (rayDirX * rayDirX))
      //deltaDistY = sqrt(1 + (rayDirX * rayDirX) / (rayDirY * rayDirY))
      //which can be simplified to abs(|rayDir| / rayDirX) and abs(|rayDir| / rayDirY)
      //where |rayDir| is the length of the vector (rayDirX, rayDirY). Its length,
      //unlike (dirX, dirY) is not 1, however this does not matter, only the
      //ratio between deltaDistX and deltaDistY matters, due to the way the DDA
      //stepping further below works. So the values can be computed as below.
      // Division through zero is prevented, even though technically that's not
      // needed in C++ with IEEE 754 floating point values.
      float deltaDistX = (rayDirX == 0) ? 1e30 : std::abs(1 / rayDirX);
      float deltaDistY = (rayDirY == 0) ? 1e30 : std::abs(1 / rayDirY);

      float perpWallDist;

      //what direction to step in x or y-direction (either +1 or -1)
      int stepX;
      int stepY;


      int hit = 0; //was there a wall hit?
      int side; //was a NS or a EW wall hit?
      //calculate step and initial sideDist
      if(rayDirX < 0)
      {
        stepX = -1;
        sideDistX = (posX - mapX) * deltaDistX;
      }
      else
      {
        stepX = 1;
        sideDistX = (mapX + 1.0 - posX) * deltaDistX;
      }
      if(rayDirY < 0)
      {
        stepY = -1;
        sideDistY = (posY - mapY) * deltaDistY;
      }
      else
      {
        stepY = 1;
        sideDistY = (mapY + 1.0 - posY) * deltaDistY;
      }
      //perform DDA
      while(hit == 0)
      {
        //jump to next map square, either in x-direction, or in y-direction
        if(sideDistX < sideDistY)
        {
          sideDistX += deltaDistX;
          mapX += stepX;
          side = 0;
        }
        else
        {
          sideDistY += deltaDistY;
          mapY += stepY;
          side = 1;
        }
        //Check if ray has hit a wall
        if(MAP_WALL_FLAG & worldMap[maze_choice][mapX][mapY]) hit = 1;
      }
      //Calculate distance projected on camera direction. This is the shortest distance from the point where the wall is
      //hit to the camera plane. Euclidean to center camera point would give fisheye effect!
      //This can be computed as (mapX - posX + (1 - stepX) / 2) / rayDirX for side == 0, or same formula with Y
      //for size == 1, but can be simplified to the code below thanks to how sideDist and deltaDist are computed:
      //because they were left scaled to |rayDir|. sideDist is the entire length of the ray above after the multiple
      //steps, but we subtract deltaDist once because one step more into the wall was taken above.
      if(side == 0) perpWallDist = (sideDistX - deltaDistX);
      else          perpWallDist = (sideDistY - deltaDistY);

      //Calculate height of line to draw on screen
      int lineHeight = (int)(VIEW_SCALE*VIEW_HEIGHT / perpWallDist);
      #ifdef RACER_DEBUG
      Serial.print("lineHeight :");
      Serial.print(lineHeight);
      Serial.print("perpWallDist :");
      Serial.println(perpWallDist);
      #endif
      //calculate lowest and highest pixel to fill in current stripe
      if (lineHeight>=VIEW_HEIGHT) lineHeight=VIEW_HEIGHT-1; // AKJ adaptation as TFT vline does 'height' rather than' 'end'
      int drawStart = -(lineHeight / 2) + VIEW_HEIGHT/2;
      //if(drawStart < 0) drawStart = 0;
      int drawEnd = (lineHeight / 2) + VIEW_HEIGHT/2; // DrawEnd is needed for textured walls
      if(drawEnd >= VIEW_HEIGHT) drawEnd = VIEW_HEIGHT - 1;

      int texNum = worldMap[maze_choice][mapX][mapY]; // Find out what the cell codes for

      // This is the start of the textured wall element

      if (texNum & MAP_TEXTURE_FLAG)
        { // The cell is textured wall
        //calculate value of wallX
        float wallX; //where exactly the wall was hit
        if (side == 0) wallX = posY + perpWallDist * rayDirY;
        else           wallX = posX + perpWallDist * rayDirX;
        wallX -= floor((wallX));

        //x coordinate on the texture
        int texX = int(wallX * float(texWidth));
        if(side == 0 && rayDirX > 0) texX = texWidth - texX - 1;
        if(side == 1 && rayDirY < 0) texX = texWidth - texX - 1;
      
            // How much to increase the texture coordinate per screen pixel
        float step = 1.0 * texHeight / lineHeight;
        // Starting texture coordinate
        float texPos = (drawStart - 0.5*VIEW_HEIGHT + 0.5*lineHeight) * step;
        for(int y = drawStart; y<drawEnd; y++)
        {
          // Cast the texture coordinate to integer, and mask with (texHeight - 1) in case of overflow
          int texY = (int)texPos & (texHeight - 1);
          texPos += step;
          uint32_t color = texture[MAP_CHOICE_MASK & texNum][texHeight * texY + texX]; // Pick pixels from the chosen texture
          //make color darker for y-sides: R, G and B byte each divided through two with a "shift" and an "and"
          //if(side == 1) color = (color >> 1) & 8355711;
          // Textures seem to need a more strident face shading
          if(side == 1) {face_shade=SIDE_SCALE*0.8;} else {face_shade=1.0;} 
          //buffer[y][x] = color;
          view.drawPixel(x,y,RGB_scale_TFT(color,(face_shade*(1.0-(perpWallDist*depth_shade)))));
        }
      } // End of wall texture code
      else
      {
        //choose wall color
// ESP32: adapt colours
        // ColorRGB color;
        int color;
        switch(MAP_CHOICE_MASK & texNum)
          {
            case 1:  color=0x00e01010;    break; //red
            case 2:  color=0x0010e010;  break; //green
            case 3:  color=0x000000ff;   break; //blue
            case 4:  color=0x00ffffff;  break; //white
            case 5:  color=0x00ffff00;  break; //yellow
            default: color=0x00808080; break; //grey

          } // End of color choice switch

        //give x and y sides different brightness
// ESP32: this shade adaptation won't work with TFT colour mapping, so we have our own shader
        if(side == 1) {face_shade=SIDE_SCALE;} else {face_shade=1.0;} 

        //draw the pixels of the stripe as a vertical line
// ESP32: Draw a verical line 
        // shaded for depth cueing
        view.drawFastVLine(x, drawStart, lineHeight, RGB_scale_TFT(color,(face_shade*(1.0-(perpWallDist*depth_shade)))));
      }
    } // End of else for non-textured walls

// ESP32: draws text on screen
    #ifdef RACER_DEBUG
    Serial.print("DirX: ");
    Serial.print(dirX);
    Serial.print("DirY: ");
    Serial.println(dirY);
    #endif
 
 // ESP32: redraws the screen buffer after everything has been placed
    view.pushSprite(0,0); // Display the view sprite on the TFT
    //redraw();
// ESP32: clears the screen to BLACK if no RGB values given
    //view.fillSprite(TFT_BLACK);
    view.fillRect(0, 0, VIEW_WIDTH, FLOOR_HORIZON, TFT_BLACK) ; // Just clear the top half, no apparent time saving
    // cls();

   //timing for input and FPS counter
    //oldtime = newtime;
  // ESP32: returns time in ms since programme started
    newtime=millis(); // Arduino system time
    // time = getTicks();
    int frameTime = newtime - startraytime; // simplified for Arduino
    #ifdef SHOW_RATE
    Serial.println(frameTime); // output the raycast time
    #endif
    //double frameTime = (time - oldTtme) / 1000.0; //frameTime is the time this frame has taken, in seconds
    // print(1.0 / frameTime); //FPS counter

#ifdef SHOW_MAZE
    draw_maze(); // show the top view after each move, perhaps this is a debug feature?
#endif
    #ifdef RACER_DEBUG
    // This is the end of raycasting so how long has it taken?
    // Without the maze and print statements it's around 12ms
    // With ll of this included it's 50ms
    tft.setTextColor(TFT_WHITE);
    tft.drawRect(160, 130, 150, 30, TFT_BLACK); // blank the text area
    tft.drawString("Raytime: "+ String(millis()-startraytime),160,130,4);
    #endif

    //speed modifiers
    //double moveSpeed = frameTime * 5.0; //the constant value is in squares/second
    //double rotSpeed = frameTime * 3.0; //the constant value is in radians/second
    float moveSpeed=0.003*(float)frameTime; // in milliseconds
    float rotSpeed=0.001*(float)frameTime;
    
    
    //readKeys();
    key_pressed=key_debounce_check_queue();
    //move forward if no wall in front of you
    // ESP32: check for set bits in the returned uint8_t
    #ifdef FOUR_BUTTONS
    if (key_pressed & BIT_CONTROL_UP)
    #else // (BIT_CONTROL_RIGHT | BIT_CONTROL_LEFT) is a generated mask for the button bit pattern
    if ((key_pressed & (BIT_CONTROL_RIGHT | BIT_CONTROL_LEFT))==(BIT_CONTROL_RIGHT | BIT_CONTROL_LEFT)) // Forwards if both down
    #endif
    {
      // if(worldMap[int(posX + dirX * moveSpeed)][int(posY)] == false) posX += dirX * moveSpeed;
      //if(worldMap[int(posX)][int(posY + dirY * moveSpeed)] == false) posY += dirY * moveSpeed;
      // It seems better if it stops once wall hit, rather than sliding along
      // and also needs to look a bigger step away so as not to become 'embedded' in the wall
      // hence the 5*dirX
      uint16_t next_cell=worldMap[maze_choice][int(posX + 5*dirX * moveSpeed)][int(posY + 5*dirY * moveSpeed)];
      if(MAP_TARGET_FLAG & next_cell)
        {
          key_pressed=0; // Make sure no button residues
          end_game(); // You have gone forwards into target so ended the game
        }
      if((MAP_WALL_FLAG & next_cell) == false)
        {
          posX += dirX * moveSpeed;
          posY += dirY * moveSpeed;
        }
    }
    //move backwards if no wall behind you - only for four buttons so may as well delete the code for two
  #ifdef FOUR_BUTTONS
  if (key_pressed & BIT_CONTROL_DOWN)
    {
      //if(worldMap[int(posX - dirX * moveSpeed)][int(posY)] == false) posX -= dirX * moveSpeed;
      //if(worldMap[int(posX)][int(posY - dirY * moveSpeed)] == false) posY -= dirY * moveSpeed;
      if((MAP_WALL_FLAG & worldMap[maze_choice][int(posX - 5*dirX * moveSpeed)][int(posY - 5*dirY * moveSpeed)]) == false)
      {
        posX -= dirX * moveSpeed;
        posY -= dirY * moveSpeed;
      }
    }
  #endif
  
    //rotate to the right
    // Rotation would work to have same code for two and four, but four allows forwards and turn simultaneously
    #ifdef FOUR_BUTTONS
    if (key_pressed & BIT_CONTROL_RIGHT)
    #else
    if ((key_pressed & (BIT_CONTROL_RIGHT | BIT_CONTROL_LEFT))==BIT_CONTROL_RIGHT) // RIGHT ONLY
    #endif
    {
      //both camera direction and camera plane must be rotated
      float oldDirX = dirX;
      dirX = dirX * cos(-rotSpeed) - dirY * sin(-rotSpeed);
      dirY = oldDirX * sin(-rotSpeed) + dirY * cos(-rotSpeed);
      float oldPlaneX = planeX;
      planeX = planeX * cos(-rotSpeed) - planeY * sin(-rotSpeed);
      planeY = oldPlaneX * sin(-rotSpeed) + planeY * cos(-rotSpeed);
    }
    //rotate to the left
    #ifdef FOUR_BUTTONS
    if (key_pressed & BIT_CONTROL_LEFT)
    #else
    if ((key_pressed & (BIT_CONTROL_RIGHT | BIT_CONTROL_LEFT))==BIT_CONTROL_LEFT) // LEFT ONLY
    #endif
    {
      //both camera direction and camera plane must be rotated
      float oldDirX = dirX;
      dirX = dirX * cos(rotSpeed) - dirY * sin(rotSpeed);
      dirY = oldDirX * sin(rotSpeed) + dirY * cos(rotSpeed);
      float oldPlaneX = planeX;
      planeX = planeX * cos(rotSpeed) - planeY * sin(rotSpeed);
      planeY = oldPlaneX * sin(rotSpeed) + planeY * cos(rotSpeed);
    }
    
  }
