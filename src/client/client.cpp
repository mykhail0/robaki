#include "client.h"
#include "to_game.h"
#include <chrono>

Client::Client(
    const std::string &player_name, int gui_sockfd, int game_sockfd,
    const SockAddr &game_srvr
) :
    id(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    ),
    player_name(player_name), gui_sockfd(gui_sockfd), game_sockfd(game_sockfd),
    game_srvr(game_srvr) {}

void Client::send_to_game() {
    Client2Game msg(id, gui.turn_direction(), buf.expected(), player_name);
    auto ret = sendto(game_sockfd, msg.data(), msg.size(), 0,
        (const sockaddr *) &game_srvr.addr, game_srvr.sz);
    if (ret == -1) {
        perror("Could not send message to game server.");
    }
}
