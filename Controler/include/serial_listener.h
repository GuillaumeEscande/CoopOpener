#ifndef SERIAL_H
#define SERIAL_H

class SerialListener {

private:
    struct handler {
        char* trigger;
        void (*function)();
    };
    uint16_t handlers_size = 0;
    handler handlers[128];
    char buffer[256];

public:
    SerialListener();
    void init();
    void on_receive(char[], void (*function)());
    void run();

};

#endif //SERIAL_H
