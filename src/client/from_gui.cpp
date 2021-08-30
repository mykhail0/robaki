#include "from_gui.h"

FromGUI::FromGUI() : turn_dir(STRAIGHT) {
    msg[LEFT_KEY_DOWN_SZ] = "LEFT_KEY_DOWN";
    turn[LEFT_KEY_DOWN_SZ] = LEFT;
    down[LEFT_KEY_DOWN_SZ] = true;

    msg[LEFT_KEY_UP_SZ] = "LEFT_KEY_UP";
    turn[LEFT_KEY_UP_SZ] = LEFT;
    down[LEFT_KEY_UP_SZ] = false;

    msg[RIGHT_KEY_DOWN_SZ] = "RIGHT_KEY_DOWN";
    turn[RIGHT_KEY_DOWN_SZ] = RIGHT;
    down[RIGHT_KEY_DOWN_SZ] = true;

    msg[RIGHT_KEY_UP_SZ] = "RIGHT_KEY_UP";
    turn[RIGHT_KEY_UP_SZ] = RIGHT;
    down[RIGHT_KEY_UP_SZ] = false;
}

ssize_t receive_data(int sockfd) {
    ssize_t len = recv(sockfd, buf, sizeof buf, 0);
    if (len < 0) {
        if (errno != EWOULDBLOCK && errno != EAGAIN)
            syserr("Reading from gui socket.\n");
        else
            perror("Blocked reading from gui.\n");
    } else {
        str += std::string(buffer, len);
    }
    return len;
}

void GUI2Game::recv(int sockfd) {
    ssize_t ret = 0;
    while (ret != -1)
        ret = receive_data(sockfd);
    auto last = str.rfind("\n");
    if (last == npos)
        return;
    last += 1;

    size_t i = str.size();
    // Direction of first parsable message from the end.
    byte_t met = STRAIGHT;
    while (true) {
        auto j = i;
        i = str.rfind("\n", i);
        if (i == npos)
            break;
        size_t len = j - i;
        auto it = msg.find(len);
        if (it == msg.end() || str.compare(i + 1, len, it->second) != 0)
            continue;

        auto t = turn[msg];
        if (down[msg]) {
            turn_dir = (met == STRAIGHT ? t : (t == met ? STRAIGHT : t));
            break;
        }
        if (met == STRAIGHT) {
            met = t;
            continue;
        }
        if (met == t)
            continue;
        // met != STRAIGHT, met != t
        // last received are up messages for L and R, so
        turn_dir = STRAIGHT;
        break;
    }
    str = str.substr(last);
    return;
}
