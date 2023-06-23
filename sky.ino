/*
* Builds a background scene that is randomised within constraints
* Importantly it must wrap-around so that it's circular
* the FOV is 2 * atan(Cameraplanevector/Directionvector)2x38.65°
* so around 0.6 degrees per pixel across the 128
*/
#include "sky.h"
#include "amaze_generic.h"

void sky_build()
{
int new_value;
int power_loop,slice,slots,spikes;

back_height[1<<(SKY_SIZE-1)]=random(VIEW_HEIGHT/3,SKY_TOP); // Set the middle and last/first to values
back_height[SKY_MASK & (1<<SKY_SIZE)]=random(SKY_BASE,SKY_TOP/2); // Could just index to zero here but this show the pattern

for(power_loop=(SKY_SIZE-2);power_loop>=0;--power_loop)
  { // Progressively loop to divide the space into smaller and small divisions
  //Serial.println(power_loop);
    slice=1<<power_loop;
    slots=1<<power_loop;
    //Serial.print("slice ");Serial.println(slice);
    spikes=power_loop*(VIEW_HEIGHT/4)/(SKY_SIZE-2); // Used to set how much variation is built into the average
    do
    {
      // Average slots 0 and 2 to make 1 with a bit of randomisation adjustment
      new_value=(random(0,spikes)-spikes/2)+(back_height[slots-slice]+back_height[SKY_MASK & (slots+slice)])/2;

      if (new_value>SKY_TOP) new_value=SKY_TOP;
      if (new_value<SKY_BASE) new_value=SKY_BASE;
      
      // Check and potentially dust with snow, store just the snow, not absolute height
      if (new_value > SKY_SNOWLINE)
        {
            snow_height[slots]=random(0,(VIEW_HEIGHT/20)); // Add a random dusting related to screen size
        }
      else snow_height[slots]=0; // No snow

      back_height[slots]=new_value;
      slots=slots+2*slice; // Then jump to 2 and 4 etc
      //delay(1500);
    }
    while (slots <= 1<<SKY_SIZE);
  }
}

void show_sky( void * parameter)
{
  for(;;) // Loop forever
  {
    // Wait until the bits say we can run
    xEventGroupWaitBits(
                       raycast_event_group,               // event group handle
                       SKY_REDRAW,                        // bits to wait for
                       pdTRUE,                            // clear the bit once we've started
                       pdFALSE,                           //  OR for any of the defined bits
                       portMAX_DELAY );                   //  block forever
  #ifdef RACER_DEBUG
Serial.println("Start of drawing the skyscape");
int skystart=micros();
#endif

// Use the sky/mountain array to build the top half of the display
// This takes between 500 and 900 microseconds dependent on mountain and snow lengths at 170 pixels square
// Start by colouring the 'sky'
  view.fillRect(0, 0, VIEW_WIDTH, VIEW_HEIGHT/2, SKY_BLUE); // This will leave a slight gap at horizon

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
      int back_index=SKY_MASK & (int)((FOV_pixel*(float)x)+start_index); // Corrected to show correct FOV, MASK does the wrap-around
      view.drawFastVLine(x, VIEW_HEIGHT/2-back_height[back_index], back_height[back_index], SKY_BROWN);
      
      // Check and maybe draw snow, from the mountain peak dowm which is increasing pixel Y position 
      if (snow_height) view.drawFastVLine(x, VIEW_HEIGHT/2-back_height[back_index], snow_height[back_index], SKY_SNOW);
    }
    #ifdef RACER_DEBUG
    int skyend=micros();
    Serial.print("sky ");Serial.println(skyend-skystart);
    #endif
    xEventGroupSetBits(
                      raycast_event_group,     // the handle
                      SKY_READY);             // set a bit to say we've rendered it
} // End of loop forever
}