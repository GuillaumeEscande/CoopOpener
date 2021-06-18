#include <Arduino.h>
#include "button.h"

#define DEBUG false

Button::Button( int pin_id):
        pin_id(pin_id) {
  function_pressed = NULL;
}


void Button::init(){
  pinMode(pin_id, INPUT);
  previous_state = digitalRead(pin_id);
}

bool Button::is_pressed(){
  return digitalRead(pin_id);
}

void Button::on_press(void (*function)()){
  function_pressed = function;
}

void Button::run(){
  bool state = digitalRead(pin_id);
  if (state && !previous_state) {
    function_pressed();
  }
  previous_state = state;
}
