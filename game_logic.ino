#include "amaze_generic.h"
#include "game_board_buttons.h"

void game_logic(){
   uint8_t key_pressed; // from the button queue

   //timing for input and FPS counter
    newtime=millis(); // Arduino system time
    int frameTime = newtime - startraytime; // simplified for Arduino
    
    //speed modifiers
    float moveSpeed=0.003*(float)frameTime; // in milliseconds
    float rotSpeed=0.001*(float)frameTime;
       
    key_pressed=key_debounce_check_queue(); // This doesn't need to go back to others as we alter the vectors
    //move forward if no wall in front of you
    // ESP32: check for set bits in the returned uint8_t
    #ifdef FOUR_BUTTONS
    if (key_pressed & BIT_CONTROL_UP)
    #else // (BIT_CONTROL_RIGHT | BIT_CONTROL_LEFT) is a generated mask for the button bit pattern
    if ((key_pressed & (BIT_CONTROL_RIGHT | BIT_CONTROL_LEFT))==(BIT_CONTROL_RIGHT | BIT_CONTROL_LEFT)) // Forwards if both down
    #endif
    {
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
#ifdef RACER_DEBUG
Serial.println("After keys, end of loop");
#endif    
}