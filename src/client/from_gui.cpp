#include "../err.h"
#include "from_gui.h"
#include <sys/types.h>
#include <sys/socket.h>

namespace {
    // Each valid message from the GUI server to the client has distinct length.
    // Here they are, minus new line character.
    constexpr size_t LEFT_KEY_DOWN_SZ = 13,
        LEFT_KEY_UP_SZ = 11,
        RIGHT_KEY_DOWN_SZ = 14,
        RIGHT_KEY_UP_SZ = 12;
}

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

void FromGUI::receive_data(int sockfd) {
    ssize_t len = 0;
    while (len != -1) {
        len = recv(sockfd, buf, sizeof buf, 0);
        if (len < 0) {
            if (errno != EWOULDBLOCK && errno != EAGAIN)
                syserr("Reading from gui socket.\n");
            else
                perror("Blocked reading from gui.\n");
        } else {
            str += std::string(buf, len);
        }
    }
}

void FromGUI::receive(int sockfd) {
    receive_data(sockfd);

    // Parse the message from the end.

    auto last = str.rfind("\n");
    if (last == std::string::npos)
        return;
    last += 1;

    size_t i = str.size();
    // Direction of first parsable message from the end.
    byte_t met = STRAIGHT;
    bool flag = true;
    while (flag) {
        auto prev_i = i;
        // Next new line character.
        i = str.rfind("\n", prev_i);

        size_t len = prev_i - (i + 1), start = i + 1;

        // If it is the last parsed segment.
        if (i == std::string::npos) {
            len = prev_i;
            start = 0;
            // Do not do the next iteration.
            flag = false;
        }

        auto it = msg.find(len);
        if (it == msg.end() || str.compare(start, len, it->second) != 0)
            // Message is not valid.
            continue;

        auto t = turn[len];
        if (down[len]) {
            // If the message is of type KEY_DOWN and is the first valid
            // message met from the end, turn_dir is this direction.
            // If the KEY_UP message was met previously,
            // turn_dir is either STRAIGHT or this direction.
            turn_dir = (met == STRAIGHT ? t : (t == met ? STRAIGHT : t));
            break;
        }
        if (met == STRAIGHT) {
            // If the message is of type KEY_UP and is the first valid message
            // met from the end, we remember this fact in `met` variable.
            met = t;
            // In case only messages of these type were sent,
            // we reset turn direction if needed.
            turn_dir = t == turn_dir ? STRAIGHT : turn_dir;
            continue;
        }
        if (met == t)
            // KEY_UP with this direction was already met previously
            // so it does not matter.
            continue;
        // We met KEY_UP messages with different directions from the end.
        turn_dir = STRAIGHT;
        break;
    }

    // Characters received after the most recently received new line character.
    str = str.substr(last);
    return;
}
