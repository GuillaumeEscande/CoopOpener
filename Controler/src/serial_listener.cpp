#include <Arduino.h>
#include "serial_listener.h"

#define DEBUG true

SerialListener::SerialListener( ) {
}


void SerialListener::init(){
  if (DEBUG) Serial.println("SerialListener::init");
  buffer = "";
}

void SerialListener::on_receive(String handler_text, void (*function)()){
  if (DEBUG) {
    Serial.print("SerialListener::on_receive ");
    Serial.print(handlers_size);
    Serial.print(" ");
    Serial.print(handler_text);
    Serial.println();
    Serial.flush();
  }
  handlers[handlers_size] = {
    handler_text,
    function
  };
  ++ handlers_size;
}

void SerialListener::run(){
  if (Serial.available()){
    if (DEBUG) Serial.println("SerialListener::run availlable");
    Serial.flush();
    String result = Serial.readString();
    Serial.flush();
    if (DEBUG) Serial.println(result);
    if (DEBUG) Serial.println("SerialListener::run result");
    Serial.flush();
    for( uint16_t i = (uint16_t)0; i < handlers_size; ++i){
        if (DEBUG){

    Serial.print("SerialListener::test handler ");
    Serial.print(i);
    Serial.print(" ");
    Serial.print(handlers[i].trigger);
    Serial.println();
    Serial.flush();
        }
      if( result.equals( handlers[i].trigger ) ){
        if (DEBUG){
          Serial.print("SerialListener::run handler found = ");
          Serial.print(i);
          Serial.println();
          Serial.flush();
        }
        handlers[i].function();
      }
    }
  }
}

