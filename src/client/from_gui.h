#ifndef FROM_GUI_H
#define FROM_GUI_H

#include "../consts.h"
#include <unordered_map>
#include <string>

class FromGUI {
private:
    byte_t turn_dir;

    // Buffer for receiving.
    char buf[1000];
    // string for parsing purposes (insted of char[]).
    // Contains characters received from the GUI server
    // after the most recently received new line character.
    std::string str;

    // Meaning of these maps is clear in the ctor.
    std::unordered_map<size_t, std::string> msg;
    std::unordered_map<size_t, byte_t> turn;
    std::unordered_map<size_t, bool> down;

public:
    FromGUI();
    // Receives data from the given socket
    // until the socket would block (EWOULDBLOCK or EAGAIN).
    // Adds received data to the `str`.
    void receive_data(int);
    // Receives data from the given socket
    // until the socket would block (EWOULDBLOCK or EAGAIN).
    // Updates turn_dir accordingly.
    void receive(int);
    byte_t turn_direction() const { return turn_dir; }
};

#endif /* FROM_GUI_H */
