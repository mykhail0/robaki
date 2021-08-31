#ifndef CLIENT_H
#define CLIENT_H

#include "from_game.h"
#include "from_gui.h"

class Client {
private:
    session_id_t id;
    std::string player_name;

    Game2GUI buf;
    FromGUI gui;

    int gui_sockfd;
    int game_sockfd;
    SockAddr game_srvr;

public:
    Client(const std::string &, int, int, const SockAddr &);

    void receive_from_game() { buf.recv(game_sockfd, game_srvr); }
    void send_to_game();
    void receive_from_gui() { gui.receive(gui_sockfd); }
    bool send_to_gui() { return buf.send(gui_sockfd); }

    int GUI() const { return gui_sockfd; }
    int game() const { return game_sockfd; }
};

#endif /* CLIENT_H */
