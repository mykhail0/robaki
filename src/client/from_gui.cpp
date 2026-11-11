#include "from_gui.h"

#include <sys/socket.h>
#include <sys/types.h>

#include <cstring>
#include <iostream>

#include "../error.h"

client::from_GUI_t::from_GUI_t() {
  // Each valid message from the GUI server to the client has distinct length,
  // so we use that to parse quickly (even different mod 4 == GUI_MSG_TYPES).
  std::string LEFT_KEY_DOWN = "LEFT_KEY_DOWN", LEFT_KEY_UP = "LEFT_KEY_UP",
              RIGHT_KEY_DOWN = "RIGHT_KEY_DOWN", RIGHT_KEY_UP = "RIGHT_KEY_UP";
  size_t i = len_to_ind(LEFT_KEY_DOWN.size());
  msg[i] = LEFT_KEY_DOWN;
  turn[i] = turn_direction_t::left;
  down[i] = true;

  i = len_to_ind(LEFT_KEY_UP.size());
  msg[i] = LEFT_KEY_UP;
  turn[i] = turn_direction_t::left;
  down[i] = false;

  i = len_to_ind(RIGHT_KEY_DOWN.size());
  msg[i] = RIGHT_KEY_DOWN;
  turn[i] = turn_direction_t::right;
  down[i] = true;

  i = len_to_ind(RIGHT_KEY_UP.size());
  msg[i] = RIGHT_KEY_UP;
  turn[i] = turn_direction_t::right;
  down[i] = false;
}

byte_t client::from_GUI_t::turn_direction() const {
  turn_direction_t ret = turn_direction_t::straight;
  if (last_pressed_left) {
    if (left_down) {
      ret = turn_direction_t::left;
    }
  } else {
    if (right_down) {
      ret = turn_direction_t::right;
    }
  }
  // Different logic:
  // if (left_down) {
  //   if (not right_down) {
  //     ret = turn_direction_t::left;
  //   }
  // } else {
  //   if (right_down) {
  //     ret = turn_direction_t::right;
  //   }
  // }
  return static_cast<byte_t>(ret);
}

void client::from_GUI_t::receive_data(const socket_t& sockfd) {
  ssize_t len = 0;
  while (len != -1) {
    len = recv(sockfd.get(), buf.data(), buf.size(), 0);
    if (len < 0) {
      if (errno == EWOULDBLOCK or errno == EAGAIN) {
        perror("Blocked reading from GUI");
      } else {
        throw critical_error(critical_error::kind::failed_recv,
                             strerror(errno));
      }
    } else {
      str.append(buf.cbegin(), buf.cbegin() + len);
    }
  }
}

void client::from_GUI_t::receive(const socket_t& sockfd) {
  receive_data(sockfd);

  std::string::size_type start = 0;
  for (auto end = str.find('\n'); end != std::string::npos;
       end = str.find('\n', start)) {
    size_t len = end - start;
    size_t ind = len_to_ind(len);
    if (str.compare(start, len, msg[ind]) == 0) {
      if (turn[ind] == turn_direction_t::left) {
        left_down = down[ind];
        if (left_down) {
          last_pressed_left = true;
        }
      } else {
        right_down = down[ind];
        if (right_down) {
          last_pressed_left = false;
        }
      }
    } else {
      std::cerr << "Invalid message from GUI" << std::endl;
    }
    start = end + 1;
  }

  // Remove characters before the last new line character.
  str = str.substr(start);
}
