#include <Arduino.h>
#include "serial_listener.h"

#define DEBUG false

SerialListener::SerialListener( ) {
}


void SerialListener::init(){
  if (DEBUG) Serial.println("SerialListener::init");
  buffer[0]= 0;
}

void SerialListener::on_receive(char handler_text[], void (*function)()){
  if (DEBUG) {
    Serial.print("SerialListener::on_receive ");
    Serial.print(handlers_size);
    Serial.print(" ");
    Serial.print(handler_text);
    Serial.println();
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
    char buffer[256];
    buffer[0] = 0;
    uint16_t cpt_buffer = 0;
    uint8_t cpt = 100;

    while((cpt_buffer == 0 || buffer[cpt_buffer - 1] != '\n') && cpt > 0){
      if (Serial.available()){
        buffer[cpt_buffer] = Serial.read();
        ++ cpt_buffer;
        buffer[cpt_buffer] = 0;
      } else {
        -- cpt;
        delay(10);
      }

    }
    if (DEBUG) {
      Serial.print("cpt = ");
      Serial.println(cpt_buffer);
      Serial.print("buffer = ");
      Serial.println(buffer);
      Serial.print("last = ");
      Serial.println((int)buffer[cpt_buffer-1]);
    }

    if ( cpt_buffer > 2 ) { 
      for( uint16_t i = (uint16_t)0; i < handlers_size; ++i){
        if (DEBUG){
          Serial.print("handler ");
          Serial.print(i);
          Serial.print(" ");
          Serial.print(handlers[i].trigger);
          Serial.println();
        }

        if(strncmp(buffer, handlers[i].trigger, cpt_buffer-2) == 0){
          if (DEBUG){
            Serial.print("SerialListener::run handler found = ");
            Serial.print(i);
            Serial.println();
          }
          handlers[i].function();
        }
      }
    }
  }
}

