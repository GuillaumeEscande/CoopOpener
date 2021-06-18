#ifndef BUTTON_H
#define BUTTON_H

class Button {

private:
    int pin_id;
    bool previous_state;

    void (*function_pressed)();

public:
    Button(int);
    void init();
    bool is_pressed();
    void on_press(void (*function)());
    void run();

};

#endif //BUTTON_H
