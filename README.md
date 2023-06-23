# Amaze
A maze game for ESP32 in Arduino framework. Makes a raycast 2.5D world. Easy to define new levels.

v1.3 of 2023-06-23 Gives dual core rendering and scaled title screens
* Cores render odd and even pixel columns to split load
* Title (and end) screen scale to match VIEW

v1.2 of 2023-06-07 Has some bugs removed and adds:
* Transparent textures
* 16 bit world map to enable more feature in the future

Whilst the algorithm for raycasting is from https://lodev.org/cgtutor/raycasting.html it has been tailored to ESP32 hardware and TFT library.

Aim of the game
---------------

Navigate through the levels of the world as quickly as possible. Levels are exited through a portal.

.h files should be modified to suit your personal situation as follows.

game_board_buttons.h
--------------------
Here you should define your ESP32 Arduino IO pins to reflect the control buttons. As default it is set for Volo's T-Display S3 gamer board. The critical ones are CONTROL_LEFT and CONTROL_RIGHT for two button operation and CONTROL_UP and CONTROL_DOWN for four buttons.

If just two buttons are defined, you move forwards by pressing CONTROL_LEFT and CONTROL_RIGHT at the same time.


amaze_generic.h
---------------
Here you can define FOUR_BUTTONS for four buttons or comment out to use just two.

VIEW_WIDTH and VIEW_HEIGHT are self-explanatory. 128 square is default and with current (lack of) optimisations a maximum size of 170 square or 150 by 200 makes sense. The title screen is only 128px square anyway so looks untidy to do more.

There are various debug and optional defines annotated.

maze.h
------
The levels are defined here. This file is fairly heavily annotated and it should be possible to use the info to make fresh levels.

At the moment, each level should be the same size and should always be surrounded by a wall.

As the world is in a .h file Amaze must be rebuilt to show changes.

textures.h
----------
Again, fairly self-explanatory.

A 16 colour palette is defined first in 32bit 0RGB format.

Textures are 64px square in 0RGB 32 bit format. The upper 8 bits are ignored currently.

The textures can be a different size, but as above, all the same, and rebuild to incorporate changes.
