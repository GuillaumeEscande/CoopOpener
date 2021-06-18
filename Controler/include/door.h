#ifndef DOOR_H
#define DOOR_H

class Door {

private:
    int motor_port_1;
    int motor_port_2;
    int sensor_open;
    int sensor_close;
    int ps_on;

    bool ask_open = false;
    bool ask_close = false;

public:
    Door(int, int, int, int, int);
    void init();
    bool is_open();
    bool is_close();
    void open();
    void close();
    void run();

};

#endif //DOOR_H
