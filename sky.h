#ifndef SKH_H
#define SKY_H

#define SKY_SIZE 10 // ie 8 bits means 256 slots
#define SKY_BASE 3
#define SKY_TOP (VIEW_HEIGHT/2 - 3)
#define SKY_SNOWLINE ((SKY_TOP*9)/10)

#define SKY_MASK ((1<<SKY_SIZE)-1)

// These 565 colours picked using http://www.barth-dev.de/online/rgb565-color-picker/
#define SKY_BROWN 0x5A42 // The mountain range
#define SKY_BLUE 0x11EE // The mountain sky
#define SKY_SNOW 0xEF7F // Mountain snow

void sky_build();
void show_sky( void * parameter);


#endif