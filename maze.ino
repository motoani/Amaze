/*
* Various functions that deal with the maze
* May 2023
*/

#include "amaze_generic.h"
#include "maze.h"

#ifdef SHOW_MAZE
TFT_eSprite maze_cell = TFT_eSprite(&tft); // definition

void maze_init() {
  maze_cell.createSprite(MAZE_PIXELS_PER_CELL,MAZE_PIXELS_PER_CELL);
  player.createSprite(MAZE_PIXELS_PER_CELL,MAZE_PIXELS_PER_CELL);
  
  player.fillSprite(TFT_BLUE);
}// end of maze_int

void draw_maze() {
  int cell_x,cell_y;
  int color;

  for (cell_y=0;cell_y<mapHeight;++cell_y) {
    for (cell_x=0;cell_x<mapWidth;++cell_x) {
      switch(MAP_CHOICE_MASK & worldMap[maze_choice][cell_y][cell_x])
      {
        case 1:  color = TFT_RED;    break; //red
        case 2:  color = TFT_GREEN;  break; //green
        case 3:  color = TFT_BLUE;   break; //blue
        case 4:  color = TFT_WHITE;  break; //white
        case 5:  color = TFT_YELLOW;  break; //yellow
        default: color = TFT_DARKGREY; break; //grey
      }
      maze_cell.fillSprite(color);
/*
      Serial.print(cell_x);
      Serial.print(" ");
      Serial.println(cell_y);
  */    
      maze_cell.pushToSprite(&mapview,cell_x*MAZE_PIXELS_PER_CELL,cell_y*MAZE_PIXELS_PER_CELL,TFT_BLACK); // push the cell sprites into the view sprite

    } //end of cell_x loop
  } //end of cell_y loop
  // lastly place the player
  // Not reversal of X and Y to postion the player, matchs the maze and walk through though!
  player.pushToSprite(&mapview,(int)posY*MAZE_PIXELS_PER_CELL,(int)posX*MAZE_PIXELS_PER_CELL,TFT_BLACK); // push the cell sprites into the view sprite

  mapview.pushSprite(160,0); // Display the view sprite on the RHS of the TFT
}// end of draw maze
#endif