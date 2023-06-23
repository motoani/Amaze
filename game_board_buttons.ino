/*
* Scan Volo's game boaud buttons via RTOS and push negative going edges
* into a queue so the main programme can consume them
* I tried a class for this but became messy due to the callback/member functions/pointers etc etc
* If there were to be more than one instance  it might be worth the struggle
* but I didn't realise what a complex issue this is!
* Simplified 2023-05-09 to remove edge detection and simply return status
* NOTE that this doesn't require FPU so doesn't invoke exception in 'lazy context switching'
*/
#include "game_board_buttons.h"

void key_wait() // Await a clear press after release
  {
    do // Now await release
      {
      delay(15);
      }
    while(key_debounce()); // Polled 

    // HOLD until button pressing  
    do // Wait until a key is pressed 
      {
      random(); // Pull randoms, uses operator time to seed
      delay(15); // A debounce period
      }
    while(!key_debounce()); // Polled 

    do // Now await release
      {
      delay(15);
      }
    while(key_debounce()); // Polled 
  }

uint8_t key_debounce() // Not driven by RTOS but includes delays
  { 
  static bool oldkeys[CONTROL_PIN_COUNT]={TRUE,TRUE,TRUE,TRUE,TRUE,TRUE};
  bool newkeys[CONTROL_PIN_COUNT];
  int pin_pos;
  uint8_t key_pressed=0;

  // Check all of the input lines; with better hardware design could be byte-wise
  // Although that wouldn't be generic for Arduino
  // It works ok if buttons pressed firmly
  for(pin_pos=0;pin_pos<CONTROL_PIN_COUNT;++pin_pos)
    { // loop through the array of pins
      // Read the hardware pin as indicated by the array
      newkeys[pin_pos]= (bool) digitalRead(control_pins[pin_pos]);
      // Check if it is low, as it was last tick, if so it's debounced
      if (!newkeys[pin_pos] && !oldkeys[pin_pos]) // AND the inverted readings as pressed is low
        { // Set the bit in the results byte by ORing 
          key_pressed=key_pressed | ((uint8_t)1<<pin_pos);
        }
      oldkeys[pin_pos]=newkeys[pin_pos]; // save for next tick
      }// End of pin for-loop
  return (key_pressed); // If there is a delay between calls this is debounced
  }// end of key debounce 



void key_debounce_init(){ // Initialise both the hardware and RTOS
  // Initialise the hardware button digital inputs
  for (uint8_t pin: control_pins) pinMode(pin,INPUT_PULLUP);

  // Setup RTOS tasks to deal with jobs
  #if defined SHOW_RATE  
  FPS_Reload_Timer = xTimerCreate(
    "TimerForFPSCOUNTER",  // Text name
    pdMS_TO_TICKS(1000),               // The interval between firing timer
    pdTRUE,                           // reload the timer after firing
    0,                                // Does not use a timer ID
    fps_callback );          // the function executed on the call back
  #endif

  Debounce_Reload_Timer = xTimerCreate(
    "TimerForControllerKeyDebounce",  // Text name
    pdMS_TO_TICKS(10),               // The interval between firing timer
    pdTRUE,                           // reload the timer after firing
    0,                                // Does not use a timer ID
    key_debounce_callback );          // the function executed on the call back

    Controller_key_queue = xQueueCreate( 5, sizeof( uint8_t ) );
 
    if(Controller_key_queue == NULL)
      {
      Serial.println("Error creating the queue");
      }                  

     if ((Debounce_Reload_Timer != NULL) && TRUE)
      {
      DRT_Started= xTimerStart(Debounce_Reload_Timer,0);     // Set the key debounce timer running
      }
      else Serial.println("Error starting debounce timer");

     #if defined SHOW_RATE  
     if ((FPS_Reload_Timer != NULL) && TRUE)
      {
      FRT_Started= xTimerStart(FPS_Reload_Timer,0);     // Set the key debounce timer running
      }
    else Serial.println("Error starting fps timer");
    #endif

} // End of init function

#if defined SHOW_RATE  
void fps_callback(TimerHandle_t xTimer)
{
  Serial.print("fps: ");Serial.println(fps);//Serial.print(" heap: ");Serial.println(ESP.getFreeHeap());  
  fps=0; // Rest the counter
}
#endif

void key_debounce_callback(TimerHandle_t xTimer)
  { 
 
  static bool oldkeys[CONTROL_PIN_COUNT]={TRUE,TRUE,TRUE,TRUE,TRUE,TRUE};
  bool newkeys[CONTROL_PIN_COUNT];
  int pin_pos;
  uint8_t key_pressed=0;

  //Serial.println("################ KEYS #################");

  // Check all of the input lines; with better hardware design could be byte-wise
  // Although that wouldn't be generic for Arduino
  // It works ok if buttons pressed firmly
  for(pin_pos=0;pin_pos<CONTROL_PIN_COUNT;++pin_pos)
    { // loop through the array of pins
      // Read the hardware pin as indicated by the array
      newkeys[pin_pos]= (bool) digitalRead(control_pins[pin_pos]);
      // Check if it is low, as it was last tick, if so it's debounced
      if (!newkeys[pin_pos] && !oldkeys[pin_pos]) // AND the inverted readings as pressed is low
        { // Set the bit in the results byte by ORing 
          key_pressed=key_pressed | ((uint8_t)1<<pin_pos);
        }
      oldkeys[pin_pos]=newkeys[pin_pos]; // save for next tick
      }// End of pin for-loop
  xQueueSend(Controller_key_queue, &key_pressed, 0); // Do not wait at all, better to skip a press than delay
  #ifdef RACER_DEBUG
  Serial.print("CONTROLLER KEY DEBOUNCE TIMER CALLBACK ");
  Serial.println(key_pressed);
  #endif
  }// end of key debounce call back

uint8_t key_debounce_check_queue(){ // See if there is anything on the queue
  //int what_was_pressed=FALSE; // defaults to false
  uint8_t key_pressed=0;

  if (pdTRUE== xQueueReceive(Controller_key_queue, &key_pressed, 0))
    { // Do not wait at all for the queue
    //Serial.print("Queue fetch");
    //what_was_pressed=control_pins[pin_pos]; // reference control buttons via array from index
    }
  //return(what_was_pressed);
  return(key_pressed); // the callback is doing the conversion from pin postion pollijng to the PIN NUMBER
} // End of queue check method
