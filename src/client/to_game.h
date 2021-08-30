#ifndef TO_GAME_H
#define TO_GAME_H

class Client2Game : public Serializable {
private:
    byte_t bytes[MAX_2_GAME_MSG_LEN];
    size_t sz;
public:
    Client2Game(session_id_t, byte_t, uint32_t, const std::string &);
    byte_t *data();
    size_t size() const { return sz; }
};

#endif /* TO_GAME_H */
