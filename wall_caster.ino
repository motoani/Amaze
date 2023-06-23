#include "amaze_generic.h"
#include "wall_caster.h"

void wall_caster(void * parameter)
{
  float face_shade; // this will adjust brightness to give a simple shade of perpendicular surfaces
  bool allow_pixel_draw[VIEW_HEIGHT]; // An array that will be set to allow drawing and cleared as the front is drawn

for(;;) { // Loop forever
   // Wait until the bits say we can run, dependent on view side
      if (*((int*)parameter)) // parameter will be zero or one
      {
    xEventGroupWaitBits(
                       raycast_event_group,               // event group handle
                       WALL_OD_REDRAW,                        // bits to wait for
                       pdTRUE,                            // clear the bit once we've started
                       pdFALSE,                           //  OR for any of the defined bits
                       portMAX_DELAY );                   //  block forever
      }
    else
      {
    xEventGroupWaitBits(
                       raycast_event_group,               // event group handle
                       WALL_EV_REDRAW,                        // bits to wait for
                       pdTRUE,                            // clear the bit once we've started
                       pdFALSE,                           //  OR for any of the defined bits
                       portMAX_DELAY );                   //  block forever
      }

    // WALL CASTING WITH SHADING AND OPTIONAL TEXTURES
    face_shade=1.0;

    for(int x = *((int*)parameter); x < VIEW_WIDTH; x=x+2) // Step across the display odd and even rows
    {
      uint16_t texNum; // A value which will be set from the world array
      bool hit, solid, poss_transp;
      // These flags are all cleared at the start of a new pixel column
      solid=false; // Set if the wall is solid ie not a transparent texture
      poss_transp=false; // Set if this column holds transparency

      #ifdef RACER_DEBUG
      Serial.print("Raycast x: ");
      Serial.println(x);
      #endif

      //calculate ray position and direction
      float cameraX = 2 * x * one_over_ViewWidth - 1; //x-coordinate in camera space
      float rayDirX = dirX + planeX * cameraX; // A lookup table was actually slower! 
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

      while (!solid) // An extra loop around DDA and pixel column drawing to keep casting until a solid wall is reached
      {
      hit=false; // Set if any wall was hit during raycasting but clear it to search for next wall
      //perform DDA
      while(!hit) // Keep doing DDA until a wall is hit by raycaster
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
        texNum = worldMap[maze_choice][mapX][mapY]; // Find out what the cell codes for, we use the variable later
        hit= MAP_WALL_FLAG & texNum; // So the loop will exit whatever type of wall is hit

      } // End of raycast DDA while loop
      // Will only exit here is the raycast has hit something

      // If it isn't a transparent wall it must be solid so set the flag to exit the DDA cycle at the 'solid' while
      solid = !(MAP_TRANSP_FLAG & texNum); // If it is transparent then DDA will be restarted after drawing

      #ifdef RACER_DEBUG
      Serial.print("X :");
      Serial.print(x);
      Serial.print("Maze map :");
      Serial.print(texNum);
      Serial.print("Hit :");
      Serial.print(hit);
      Serial.print("Solid :");
      Serial.println(solid);
      #endif

      // Set a flag to say it's a transparency column, which is only cleared at next column
      if (!poss_transp  && !solid)
        {
        #ifdef RACER_DEBUG
        Serial.print("Set pos_transp :");
        Serial.println(texNum);
        #endif
          poss_transp=true;
          // As it's a transparent texture we need to invoke the transparency buffer
          // Start by setting every pixel to true, only do it for affected columns to save time on purely solid cells
          for (int looper=0;looper<VIEW_HEIGHT;++looper) allow_pixel_draw[looper]=true;
        }
      // else poss_transp=false is infered as it's cleared at the beginning of the loop

      //Calculate distance projected on camera direction. This is the shortest distance from the point where the wall is
      //hit to the camera plane. Euclidean to center camera point would give fisheye effect!
      //This can be computed as (mapX - posX + (1 - stepX) / 2) / rayDirX for side == 0, or same formula with Y
      //for size == 1, but can be simplified to the code below thanks to how sideDist and deltaDist are computed:
      //because they were left scaled to |rayDir|. sideDist is the entire length of the ray above after the multiple
      //steps, but we subtract deltaDist once because one step more into the wall was taken above.


      if(side == 0) perpWallDist = (sideDistX - deltaDistX);
      else          perpWallDist = (sideDistY - deltaDistY);

      #ifdef RACER_DEBUG_2
      Serial.print("Side ");Serial.print(side);Serial.print(" sideDist ");Serial.print(sideDistX);Serial.print(" ");Serial.print(sideDistY);
      Serial.print(" deltaDist ");Serial.print(deltaDistX);Serial.print(" ");Serial.print(deltaDistY);Serial.print(" Perp");Serial.println(perpWallDist);
      key_wait();
      #endif

      // Calculate height of line to draw on screen
      // Adjusted to fit the VIEW aspect ratio
      int lineHeight = (int)(aspect_correct / perpWallDist);
      #ifdef RACER_DEBUG
      Serial.print("lineHeight :");
      Serial.print(lineHeight);
      Serial.print("perpWallDist :");
      Serial.println(perpWallDist);
      #endif
      //calculate lowest and highest pixel to fill in current stripe
      // Whilst the TFT library is supposed to be safe to read out of range,
      // at close up the transparency buffer is written out of range
      //if (lineHeight>=VIEW_HEIGHT) lineHeight=VIEW_HEIGHT-1; // AKJ adaptation as TFT vline does 'height' rather than' 'end'
      int drawStart = -(lineHeight / 2) + VIEW_HEIGHT/2;
      if(drawStart < 0) drawStart = 0;
      int drawEnd = (lineHeight / 2) + VIEW_HEIGHT/2; // DrawEnd is needed for textured walls
      if(drawEnd >= VIEW_HEIGHT) drawEnd = VIEW_HEIGHT - 1;

      switch (texNum & (MAP_WALL_FLAG | MAP_TEXTURE_FLAG | MAP_TRANSP_FLAG)) // Mask off everything but the various wall flags
      {
      case MAP_WALL_FLAG | MAP_TEXTURE_FLAG: // handle an ordinary textured wall
      #ifdef RACER_DEBUG
      Serial.println("Textured solid wall cell");
      #endif
      // This is the start of the textured wall element
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

        for(int y = drawStart; y<drawEnd; y++) // The max and min limit draw range and array entries
        {
          // Cast the texture coordinate to integer, and mask with (texHeight - 1) in case of overflow
          int texY = (int)texPos & (texHeight - 1);
          texPos += step;
          uint32_t color = texture[MAP_CHOICE_MASK & texNum][texHeight * texY + texX]; // Pick pixels from the chosen texture

          // Textures seem to need a more strident face shading
          if(side == 1) {face_shade=SIDE_SCALE*0.8;} else {face_shade=1.0;} 
 
          if (poss_transp) // We are on a transparent column && allow_pixel_draw[y]) // We are looking through transparent pixels
            {
              if (allow_pixel_draw[y])
                {
                  // Draw the texture behind as indicated
                  view.drawPixel(x,y,RGB_scale_TFT(color,(face_shade*(1.0-(perpWallDist*depth_shade)))));
                  allow_pixel_draw[y]=false; // Clear it now we've drawn in it
                }
            }
          else
            { // Not a transparent column so draw wahtever
              view.drawPixel(x,y,RGB_scale_TFT(color,(face_shade*(1.0-(perpWallDist*depth_shade))))); // An ordinary texture draw
            }
        } // End of column texture draw
        break;
      } // End of wall texture code

      case MAP_TRANSP_FLAG | MAP_WALL_FLAG | MAP_TEXTURE_FLAG:
      #ifdef RACER_DEBUG
      Serial.println("Transparent textured wall cell");
      #endif
      { // We are here with a transparent textured cell at this scan line
        // This could be the first transparent texture cell or one behind it 
        // Code copied from the ordinary textured block
        //calculate value of wallX
        float wallX; //where exactly the wall was hit

        if (((texNum & MAP_RUNS_NORTH) && side==1)|| (!(texNum & MAP_RUNS_NORTH) && side==0) )
        {// Only draw '1' side if runs N-S, and '0' if runs E-W

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

          // Textures seem to need a more strident face shading than solid colours
          if(side == 1) {face_shade=SIDE_SCALE*0.8;} else {face_shade=1.0;} 
 
          if (color)
          { // Never draw if the pixel is coloured as 0x0000 ie transparent
            if (poss_transp && allow_pixel_draw[y]) // We are looking through transparent pixels
            {
              view.drawPixel(x,y,RGB_scale_TFT(color,(face_shade*(1.0-(perpWallDist*depth_shade))))); // An ordinary texture draw
              allow_pixel_draw[y]=false; // Clear it now we've drawn in it, this will be set from front backwards
            }
          } // End of if-color
          // else its 0x0000 which is transparent so draw nothing and allow subsequent pixel draws by default
        } // End of column texture draw     
        #ifdef RACER_DEBUG
        Serial.println("End of transparent textured wall pixel column");
        #endif
        } // End of only draw the side
        break;
      } // End of a transparent texture block code

      case MAP_WALL_FLAG: // Handle a plain wall
      #ifdef RACER_DEBUG
      Serial.println("Plain wall cell");
      #endif

      {
        //choose wall color
        int color=palette[MAP_CHOICE_MASK & texNum];

        //give x and y sides different brightness
        if(side == 1) {face_shade=SIDE_SCALE;} else {face_shade=1.0;} 

        //draw the pixels of the stripe as a vertical line
        // shaded for depth cueing
        // If the scan column is not transparent then just draw a line
        if (!poss_transp) view.drawFastVLine(x, drawStart, lineHeight, RGB_scale_TFT(color,(face_shade*(1.0-(perpWallDist*depth_shade))))); // No transparency issues
        else
        //if (poss_transp) // We are looking through transparent potentially, place a pixel if it can be seen
          {
          for(int yl = drawStart; yl<drawEnd; yl++)
            {
              if (allow_pixel_draw[yl])
               {
                // Draw the solid colour behind as indicated
                view.drawPixel(x,yl,RGB_scale_TFT(color,(face_shade*(1.0-(perpWallDist*depth_shade))))); // An ordinary texture draw
                allow_pixel_draw[yl]=false; // Clear it now we've drawn in it, this will be set from front backwards
              }
            } // End of loop to draw a line with transparency masking
          } // End of if-else for transparency
      }  // End of else for non-textured walls
      } // End of wall SWITCH/CASE
      } // End of outer DDA 'solid' while loop for transparency
    } // End of X loop for pixel columns
    // Wall raycasting is done when this point is reached

    if (*((int*)parameter))
      {
        xEventGroupSetBits(
                      raycast_event_group,     // the handle
                      WALL_OD_READY);             // set a bit to say we've rendered the right side
      }
    else
      {
        xEventGroupSetBits(
                      raycast_event_group,     // the handle
                      WALL_EV_READY);             // set a bit to say we've rendered the left side
      }
} // End of loop forever
} // End of wall_caster function