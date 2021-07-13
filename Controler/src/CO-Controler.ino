
#include <Thread.h>
#include <ThreadController.h>
#include "protocol.hpp"
#include "door.h"
#include "button.h"
#include "serial_listener.h"

#define DEBUG false

#define INTERNAL_SWITCH_OPEN_PIN 12
#define INTERNAL_SWITCH_CLOSE_PIN 11
#define INTERNAL_MOTOR_1_PIN A3
#define INTERNAL_MOTOR_2_PIN A2

#define EXTERNAL_SWITCH_OPEN_PIN 8  
#define EXTERNAL_SWITCH_CLOSE_PIN 7
#define EXTERNAL_MOTOR_1_PIN A1
#define EXTERNAL_MOTOR_2_PIN A0


#define SWITCH_COMMAND_OPEN 10
#define SWITCH_COMMAND_CLOSE 9
#define PS_ON 3



ThreadController controller = ThreadController();

Thread motorThread = Thread();
Thread serialThread = Thread();
Thread buttonThread = Thread();

Door internal(
  INTERNAL_MOTOR_1_PIN,
  INTERNAL_MOTOR_2_PIN,
  INTERNAL_SWITCH_OPEN_PIN,
  INTERNAL_SWITCH_CLOSE_PIN, 
  PS_ON);

Door external(
  EXTERNAL_MOTOR_1_PIN,
  EXTERNAL_MOTOR_2_PIN,
  EXTERNAL_SWITCH_OPEN_PIN,
  EXTERNAL_SWITCH_CLOSE_PIN, 
  PS_ON);

Button open_switch(SWITCH_COMMAND_OPEN);
Button close_switch(SWITCH_COMMAND_CLOSE);

SerialListener listener;

void open_door(){
  if (DEBUG)  Serial.println("open_door");
  internal.open();
  //external.open();
}

void close_door(){
  if (DEBUG)  Serial.println("close_door");
  internal.close();
  //external.close();
}

void setup(void) {

  Serial.begin(57600);
  if (DEBUG) Serial.println("INIT - Start");


  // Init stepper motor
  internal.init();
  external.init();
  open_switch.init();
  close_switch.init();
  listener.init();
  listener.on_receive(QUERY_STATE, [](){
    if ( internal.is_open() ){
      Serial.println(RESPONSE_IS_OPEN);
    } else if ( internal.is_open() ){
      Serial.println(RESPONSE_IS_CLOSE);
    } else {
      Serial.println(RESPONSE_IS_UNKNOW);
    }
  });
  listener.on_receive(ACTION_OPEN, open_door);
  listener.on_receive(ACTION_CLOSE, close_door);

  open_switch.on_press(open_door);

  close_switch.on_press(close_door);


  motorThread.onRun( []() {
    internal.run();
    external.run();
  });

  serialThread.onRun( []() {
      listener.run();
  });


  buttonThread.onRun( []() {
    open_switch.run();
    close_switch.run();
  });

  serialThread.setInterval(100);
  buttonThread.setInterval(100);
  motorThread.setInterval(100);
  
  controller.add(&motorThread); 
  controller.add(&serialThread); 
  controller.add(&buttonThread); 

  if (DEBUG) Serial.println("INIT - Stop");
  
}

void loop(void) {
  controller.run();
}
