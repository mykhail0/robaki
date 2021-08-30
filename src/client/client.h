#ifndef CLIENT_H
#define CLIENT_H

class Client {
    session_id_t id;
    std::string player_name;

    Game2GUI buf;
    FromGUI gui;

    int gui_sockfd;
    int game_sockfd;
    SockAddr game_srvr;

    void receive_from_game() { buf.recv(game_sockfd, game_srvr); }
    void send_to_game();
    void receive_from_gui() { gui.recv(gui_sockfd); }
    bool send_to_gui() { return buf.send(gui_sockfd); }
};

#endif /* CLIENT_H */
