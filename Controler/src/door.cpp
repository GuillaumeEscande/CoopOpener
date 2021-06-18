#include <Arduino.h>
#include "door.h"

#define DEBUG false

Door::Door( int motor_port_1, int motor_port_2, int sensor_open, int sensor_close, int ps_on):
        motor_port_1(motor_port_1),
        motor_port_2(motor_port_2),
        sensor_open(sensor_open),
        sensor_close(sensor_close),
        ps_on(ps_on) { }


void Door::init(){
  pinMode(sensor_open, INPUT);
  pinMode(sensor_close, INPUT);
  pinMode(motor_port_1, OUTPUT);
  pinMode(motor_port_2, OUTPUT);
  pinMode(ps_on, OUTPUT);
  
  digitalWrite(motor_port_1, LOW);
  digitalWrite(motor_port_2, LOW);
  digitalWrite(ps_on, HIGH);
}


bool Door::is_open(){
   
  if (DEBUG) {
    if (!digitalRead(sensor_open)) Serial.println("Door::is_open = true");
    else Serial.println("Door::is_open = false");
  }

  return !digitalRead(sensor_open);
}

bool Door::is_close(){

  if (DEBUG) {
    if (!digitalRead(sensor_close)) Serial.println("Door::is_close = true");
    else Serial.println("Door::is_close = false");
  }

  return !digitalRead(sensor_close);
}

void Door::open(){
  ask_open = true;
  ask_close = false;
  digitalWrite(ps_on, LOW);
}

void Door::close(){
  if (DEBUG) Serial.println("Door::close");
  ask_close = true;
  ask_open = false;
  digitalWrite(ps_on, LOW);
}

void Door::run(){
  if(ask_open){
    if (is_open()) {
      digitalWrite(ps_on, HIGH);
      ask_open = false;
    } else {
        digitalWrite(motor_port_1, LOW);
        digitalWrite(motor_port_2, HIGH);
    }
  } else if(ask_close) {
    if (is_close()) {
      digitalWrite(ps_on, HIGH);
      ask_close = false;
    } else {
        digitalWrite(motor_port_1, HIGH);
        digitalWrite(motor_port_2, LOW);
    }
  } else {
        digitalWrite(motor_port_1, LOW);
        digitalWrite(motor_port_2, LOW);
  }
}

