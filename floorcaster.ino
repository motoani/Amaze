#include "amaze_generic.h"
#include "maze.h"
#include "textures.h"

void floorcaster(void * parameter)
{
for (;;) // loop forever 
{ 
  // Wait until the bits say we can run
    xEventGroupWaitBits(
                       raycast_event_group,               // event group handle
                       FLOOR_REDRAW,                      // bits to wait for
                       pdTRUE,                            // clear the bit once we've started
                       pdFALSE,                           //  OR for any of the defined bits
                       portMAX_DELAY );                   //  block forever


for(int y = FLOOR_HORIZON; y < VIEW_HEIGHT; y++)  // Added h/2 to miss the ceiling 3+ to avoid the horizontal rays
    {
      #ifdef RACER_DEBUG
      Serial.print("Floor cast pixel column is: ");Serial.println(y);
      #endif


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
      float floorStepX = rowDistance * (rayDirX1 - rayDirX0) * one_over_ViewWidth; // Swapped a divide for a multiply by the constant reciprocal
      float floorStepY = rowDistance * (rayDirY1 - rayDirY0) * one_over_ViewWidth;

      // real world coordinates of the leftmost column. This will be updated as we step to the right.
      float floorX = posX + rowDistance * rayDirX0;
      float floorY = posY + rowDistance * rayDirY0;

      for(int x = 0; x < VIEW_WIDTH; ++x)
      {
        // the cell coord is simply got from the integer parts of floorX and floorY
        int cellX = (int)(floorX);
        int cellY = (int)(floorY);

        uint16_t texNum = worldMap[maze_choice][cellX][cellY]; // Find out what is in the cell
        unsigned long color;
        if (!(texNum & MAP_WALL_FLAG)) // It's a wall, not a floor, so skip mapping into a tile and sending pixels
        // If it's a transparent block then some floor errors may be seen - make it higher!
        // This does mean that some sections of the floor are not drawn but they should be covered with wall
        // Except for some transparent designs 
        // This should also save time especially when they are textured
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
            color=palette[MAP_CHOICE_MASK & texNum];
          } // End of plain colour fill
        // Plot the colored floor pixel and also do depth cue shading  
        view.drawPixel(x,y,RGB_scale_TFT(color,1.0-(rowDistance*depth_shade)));
        } // End of deciding if it really is floor

        floorX += floorStepX;
        floorY += floorStepY;
      }
    }
    xEventGroupSetBits(
                    raycast_event_group,     // the handle
                    FLOOR_READY);             // set a bit to say we've rendered it

} // End of loop forever
} // End of floorcaster function