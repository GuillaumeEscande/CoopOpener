#ifndef SERIAL_H
#define SERIAL_H

class SerialListener {

private:
    struct handler {
        String trigger;
        void (*function)();
    };
    uint16_t handlers_size = 0;
    handler handlers[128];
    String buffer;

public:
    SerialListener();
    void init();
    void on_receive(String, void (*function)());
    void run();

};

#endif //SERIAL_H
