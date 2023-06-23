/*
* The routines here process the starter screen which is 128x128 565 to suit the VIEW dimensions and converts back to TFT 16bit colours
* bilinear transformation is derived from that at https://github.com/MalcolmMcLean/babyxrc/blob/master/src/resize.c
*/
#include "maze_title.h"

//  resize an image using bilinear interpolation
// TFT 16 bit converted to 3x8bit RGB, transformed and then back to TFT
void bilerp(uint16_t *dest, int dwidth, int dheight, uint16_t *src, int swidth, int sheight, bool swap)
{
  uint8_t red8, green8, blue8; // temporary 24 bit colour
  float a, b;
  float red, green, blue; //float alpha;
  float dx, dy;
  float rx, ry;
  int x, y;
  int index0, index1, index2, index3;

  dx = ((float) swidth)/dwidth;
  dy = ((float) sheight)/dheight;
  for(y=0, ry = 0;y<dheight-1;y++, ry += dy)
  {
    b = ry - (int) ry;
    for(x=0, rx = 0;x<dwidth-1;x++, rx += dx)
    {
      a = rx - (int) rx;
      index0 = (int)ry * swidth + (int) rx;
      index1 = index0 + 1;
      index2 = index0 + swidth;     
      index3 = index0 + swidth + 1;

      TFT_to_RGB(src[index0], &red8, &green8, &blue8,swap);
      red=red8 * (1.0f-a)*(1.0f-b);
      green=green8 * (1.0f-a)*(1.0f-b);
      blue=blue8 * (1.0f-a)*(1.0f-b);
      
      TFT_to_RGB(src[index1], &red8, &green8, &blue8,swap);
      red += red8 * (a)*(1.0f-b);
      green += green8 * (a)*(1.0f-b);
      blue += blue8 * (a)*(1.0f-b);

      TFT_to_RGB(src[index2], &red8, &green8, &blue8,swap);
      red += red8 * (1.0f-a)*(b);
      green += green8 * (1.0f-a)*(b);
      blue += blue8 * (1.0f-a)*(b);

      TFT_to_RGB(src[index3], &red8, &green8, &blue8,swap);
      red += red8 * (a)*(b);
      green += green8 * (a)*(b);
      blue += blue8 * (a)*(b);

      red = red < 0 ? 0 : red > 255 ? 255 : red;
      green = green < 0 ? 0 : green > 255 ? 255 : green;
      blue = blue < 0 ? 0 : blue > 255 ? 255 : blue;

      dest[y*dwidth+x] = RGB_to_TFT(red, green, blue,swap);

    }
    index0 = (int)ry * swidth + (int) rx;
    index1 = index0;
    index2 = index0 + swidth;     
    index3 = index0 + swidth;   

    TFT_to_RGB(src[index0], &red8, &green8, &blue8,swap);
    red = red8 * (1.0f-a)*(1.0f-b);
    green = green8 * (1.0f-a)*(1.0f-b);
    blue = blue8 * (1.0f-a)*(1.0f-b);

    TFT_to_RGB(src[index1], &red8, &green8, &blue8,swap);
    red += red8 * (a)*(1.0f-b);
    green += green8 * (a)*(1.0f-b);
    blue += blue8 * (a)*(1.0f-b);

    TFT_to_RGB(src[index2], &red8, &green8, &blue8,swap);
    red += red8 * (1.0f-a)*(b);
    green += green8 * (1.0f-a)*(b);
    blue += blue8 * (1.0f-a)*(b);

    TFT_to_RGB(src[index3], &red8, &green8, &blue8,swap);
    red += red8 * (a)*(b);
    green += green8 * (a)*(b);
    blue += blue8 * (a)*(b);
        
    red = red < 0 ? 0 : red > 255 ? 255 : red;
    green = green < 0 ? 0 : green > 255 ? 255 : green;
    blue = blue < 0 ? 0 : blue > 255 ? 255 : blue;

    dest[y*dwidth+x] = RGB_to_TFT(red, green, blue,swap);
  }
  index0 = (int)ry * swidth + (int) rx;
  index1 = index0;
  index2 = index0 + swidth;     
  index3 = index0 + swidth;   

  for(x=0, rx = 0;x<dwidth-1;x++, rx += dx)
  {
    a = rx - (int) rx;
    index0 = (int)ry * swidth + (int) rx;
    index1 = index0 + 1;
    index2 = index0;     
    index3 = index0;

    TFT_to_RGB(src[index0], &red8, &green8, &blue8,swap);
    red = red8 * (1.0f-a)*(1.0f-b);
    green = green8 * (1.0f-a)*(1.0f-b);
    blue = blue8 * (1.0f-a)*(1.0f-b);

    TFT_to_RGB(src[index1], &red8, &green8, &blue8,swap);
    red += red8 * (a)*(1.0f-b);
    green += green8 * (a)*(1.0f-b);
    blue += blue8 * (a)*(1.0f-b);

    TFT_to_RGB(src[index2], &red8, &green8, &blue8,swap);
    red += red8 * (1.0f-a)*(b);
    green += green8 * (1.0f-a)*(b);
    blue += blue8 * (1.0f-a)*(b);

    TFT_to_RGB(src[index3], &red8, &green8, &blue8,swap);
    red += red8 * (a)*(b);
    green += green8 * (a)*(b);
    blue += blue8 * (a)*(b);

    red = red < 0 ? 0 : red > 255 ? 255 : red;
    green = green < 0 ? 0 : green > 255 ? 255 : green;
    blue = blue < 0 ? 0 : blue > 255 ? 255 : blue;
      
    dest[y*dwidth+x] = RGB_to_TFT(red, green, blue,swap);
  }

  TFT_to_RGB(src[(sheight-1)*swidth+swidth-1], &red8, &green8, &blue8,swap);
  
  dest[y*dwidth+x] = RGB_to_TFT(red8, green8, blue8,swap);
}  
