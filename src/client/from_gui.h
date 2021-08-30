#ifndef CLIENT_H
#define CLIENT_H

constexpr size_t LEFT_KEY_DOWN_SZ = 13,
    LEFT_KEY_UP = 11,
    RIGHT_KEY_DOWN_SZ = 14,
    RIGHT_KEY_UP_SZ = 12;

class FromGUI {
private:
    byte_t turn_dir;

    char buf[1000];
    std::string str;

    std::unordered_map<size_t, std::string> msg;
    std::unordered_map<size_t, byte_t> turn;
    std::unordered_map<size_t, bool> down;

public:
    FromGUI();
    ssize_t receive_data(int);
    void recv(int);
    byte_t turn_direction() const { return turn_dir; }
};

#endif /* CLIENT_H */
