#include "client.h"

void Client::send_to_game() {
    Client2Game msg(id, gui.turn_direction(), buf.expected(), player_name);
    auto ret = sendto(game_sockfd, msg.data(), msg.size(), 0,
        (const sockaddr *) &game_srvr.addr, game_srvr.sz);
    if (ret == -1) {
        perror("Could not send message to game server.");
    }
}
