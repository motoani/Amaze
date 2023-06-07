// To give a closure screen at the end of the game
#include "end_game.h"
unsigned long end_game_time,game_minutes,game_seconds;
char time_out[6]; // Build a string to display

void end_game() // This is called when we pass through a portal
  {
  int i,x,xn,y; // indices to fill the screen
  #ifdef RACER_DEBUG
  Serial.println("Entered end_game");
  #endif

    maze_choice++; // Move to the next level
    if (maze_choice<MAX_MAZE_CHOICES)
      {
        for (i=0;i<10;++i) // Make random fuzz on the screen as a transition
        {
           for (y=0;y<VIEW_HEIGHT;++y)
              {
              x=-random(0,10);
              do
                {
                  xn=random(1,VIEW_WIDTH/8); // A random line length
                  view.drawFastHLine(x,y,xn,random(0,0xffff)); // Random 565 colour
                  x=x+xn;
                }
              while(x<VIEW_WIDTH);
              }
          view.pushSprite(0,0); // Display the view sprite on the TFT
        } // End of random noise loop
        player_position_reset(); // Move the player back to the start and initiate view and camera planes
      } // End of if for next level
    else
    { // This means that we have completed the game
      game_duration=millis()-game_start_time; // How long have we been playing for?
      game_minutes=(int)game_duration/60000;
      game_seconds=(int)(game_duration-(game_minutes*60000))/1000;

#ifdef RACER_DEBUG
      Serial.println(game_minutes);
      Serial.println(game_seconds);
#endif
      // Show our little backround image
      tft.pushImage(0,0,128,128,maze_title); // Show the startup image for the game once again

      tft.drawString("Time:",VIEW_WIDTH/2,VIEW_HEIGHT/6); // A heading

      time_out[0]='0'+(game_minutes/10);
      time_out[1]='0'+(game_minutes%10);

      time_out[2]='.'; // Colon seperator

      time_out[3]='0'+(game_seconds/10);
      time_out[4]='0'+(game_seconds%10);

      time_out[5]=0x00; //zero terminator

      tft.drawString(time_out,VIEW_WIDTH/2,VIEW_HEIGHT/2); // Show the game length

      key_wait(); // Await user pressing before quitting
      delay(2500); // Just wait
      tft.fillScreen(TFT_BLACK); // clear the screen 

      ESP.restart(); // Start at the very begining of the game, crude but functional
    }
  }

