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
      //Serial.print("Slots ");Serial.print(slots-slice);Serial.print(" ");Serial.println(slots+slice);
      //Serial.print(back_height[slots-slice]);Serial.print(" ");
      //Serial.println(back_height[SKY_MASK & (slots+slice)]);
      
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
  //Serial.println("Next set");
  }
}