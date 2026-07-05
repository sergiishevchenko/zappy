#ifndef GFX_HPP
#define GFX_HPP

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <thread>
#include <atomic>
#include <deque>
#include <cstdint>
#include <time.h>

inline int64_t now_ms()
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

struct Tile {
    int res[7] = {};
};

struct Player {
    int id = -1;
    int x = 0;
    int y = 0;
    int orient = 1;
    int level = 1;
    std::string team;
    bool alive = true;
};

struct Egg {
    int id = -1;
    int player_id = -1;
    int x = 0;
    int y = 0;
};

struct Incantation {
    int x = 0;
    int y = 0;
    int level = 0;
    int64_t until_ms = 0;
    std::vector<int> players;
};

struct Broadcast {
    int player_id = -1;
    int x = 0;
    int y = 0;
    int orient = 1;
    std::string text;
    int64_t start_ms = 0;
    int64_t until_ms = 0;
};

struct ServerMsg {
    std::string text;
    int64_t until_ms;
};

struct GameState {
    int width = 0;
    int height = 0;
    int freq = 100;
    std::vector<std::vector<Tile>> map;
    std::map<int, Player> players;
    std::map<int, Egg> eggs;
    std::vector<std::string> teams;
    std::string winner;
    std::deque<Broadcast> broadcasts;
    std::deque<ServerMsg> server_msgs;
    std::deque<Incantation> incants;
    std::mutex mtx;

    void resize(int w, int h);
    void set_tile(int x, int y, const int *res);
};

class NetClient {
public:
    NetClient(const std::string &host, int port);
    ~NetClient();
    bool connect();
    void disconnect();
    void send_cmd(const std::string &cmd);
    GameState &state() { return state_; }
    bool connected() const { return connected_; }

private:
    void reader_loop();
    void handle_line(const std::string &line);

    std::string host_;
    int port_;
    int sock_ = -1;
    std::atomic<bool> running_{false};
    std::thread reader_;
    GameState state_;
    bool connected_ = false;
    std::string buf_;
    std::mutex send_mtx_;
};

int run_gfx(const char *host, int port);

#endif
