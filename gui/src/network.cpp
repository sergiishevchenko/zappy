#include "gfx.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

void GameState::resize(int w, int h)
{
    width = w;
    height = h;
    map.assign(h, std::vector<Tile>(w));
}

void GameState::set_tile(int x, int y, const int *res)
{
    if (y < 0 || y >= height || x < 0 || x >= width)
        return;
    for (int i = 0; i < 7; i++)
        map[y][x].res[i] = res[i];
}

NetClient::NetClient(const std::string &host, int port)
    : host_(host), port_(port) {}

NetClient::~NetClient()
{
    disconnect();
}

void NetClient::send_cmd(const std::string &cmd)
{
    std::lock_guard<std::mutex> lk(send_mtx_);
    if (sock_ >= 0) {
        std::string line = cmd;
        if (line.empty() || line.back() != '\n')
            line += '\n';
        send(sock_, line.c_str(), line.size(), 0);
    }
}

bool NetClient::connect()
{
    struct sockaddr_in addr;
    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ < 0)
        return false;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    if (inet_pton(AF_INET, host_.c_str(), &addr.sin_addr) <= 0)
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    if (::connect(sock_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock_);
        sock_ = -1;
        return false;
    }
    running_ = true;
    reader_ = std::thread(&NetClient::reader_loop, this);
    connected_ = true;
    send_cmd("GRAPHIC");
    usleep(200000);
    send_cmd("msz");
    send_cmd("mct");
    send_cmd("tna");
    send_cmd("sgt");
    return true;
}

void NetClient::disconnect()
{
    running_ = false;
    if (sock_ >= 0) {
        shutdown(sock_, SHUT_RDWR);
        close(sock_);
        sock_ = -1;
    }
    if (reader_.joinable())
        reader_.join();
    connected_ = false;
}

void NetClient::handle_line(const std::string &line)
{
    if (line.empty() || line == "WELCOME" || line == "BIENVENUE")
        return;
    std::lock_guard<std::mutex> lk(state_.mtx);
    if (line.rfind("msz ", 0) == 0) {
        int w, h;
        if (sscanf(line.c_str(), "msz %d %d", &w, &h) == 2)
            state_.resize(w, h);
        return;
    }
    if (line.rfind("bct ", 0) == 0) {
        int x, y, r[7];
        if (sscanf(line.c_str(), "bct %d %d %d %d %d %d %d %d %d",
                &x, &y, &r[0], &r[1], &r[2], &r[3], &r[4], &r[5], &r[6]) == 9) {
            if (state_.width == 0)
                state_.resize(1, 1);
            state_.set_tile(x, y, r);
        }
        return;
    }
    if (line.rfind("tna ", 0) == 0) {
        state_.teams.push_back(line.substr(4));
        return;
    }
    if (line.rfind("sgt ", 0) == 0 || line.rfind("sst ", 0) == 0) {
        int t;
        if (sscanf(line.c_str() + 4, "%d", &t) == 1)
            state_.freq = t;
        return;
    }
    if (line.rfind("smg ", 0) == 0) {
        ServerMsg m{line.substr(4), now_ms() + 5000};
        state_.server_msgs.push_back(m);
        if (state_.server_msgs.size() > 10)
            state_.server_msgs.pop_front();
        return;
    }
    if (line.rfind("pnw ", 0) == 0) {
        Player p;
        char team[128];
        if (sscanf(line.c_str(), "pnw #%d %d %d %d %d %127s",
                &p.id, &p.x, &p.y, &p.orient, &p.level, team) == 6) {
            p.team = team;
            p.alive = true;
            state_.players[p.id] = p;
        }
        return;
    }
    if (line.rfind("ppo ", 0) == 0) {
        int id, x, y, o;
        if (sscanf(line.c_str(), "ppo #%d %d %d %d", &id, &x, &y, &o) == 4) {
            auto it = state_.players.find(id);
            if (it != state_.players.end()) {
                it->second.x = x;
                it->second.y = y;
                it->second.orient = o;
            }
        }
        return;
    }
    if (line.rfind("plv ", 0) == 0) {
        int id, l;
        if (sscanf(line.c_str(), "plv #%d %d", &id, &l) == 2) {
            auto it = state_.players.find(id);
            if (it != state_.players.end())
                it->second.level = l;
        }
        return;
    }
    if (line.rfind("pdi ", 0) == 0) {
        int id = atoi(line.c_str() + 4);
        auto it = state_.players.find(id);
        if (it != state_.players.end())
            it->second.alive = false;
        return;
    }
    if (line.rfind("pbc ", 0) == 0) {
        int id = atoi(line.c_str() + 4);
        size_t sp = line.find(' ', 4);
        std::string msg = sp != std::string::npos ? line.substr(sp + 1) : "";
        Broadcast b{id, msg, now_ms() + 3000};
        state_.broadcasts.push_back(b);
        if (state_.broadcasts.size() > 20)
            state_.broadcasts.pop_front();
        return;
    }
    if (line.rfind("pic ", 0) == 0) {
        Incantation inc;
        inc.until_ms = now_ms() + 5000;
        char buf[512];
        strncpy(buf, line.c_str(), sizeof(buf) - 1);
        buf[sizeof(buf) - 1] = '\0';
        char *tok = strtok(buf + 4, " ");
        if (tok) inc.x = atoi(tok);
        tok = strtok(NULL, " ");
        if (tok) inc.y = atoi(tok);
        tok = strtok(NULL, " ");
        if (tok) inc.level = atoi(tok);
        while ((tok = strtok(NULL, " ")) != NULL) {
            if (tok[0] == '#')
                inc.players.push_back(atoi(tok + 1));
        }
        state_.incants.push_back(inc);
        return;
    }
    if (line.rfind("pie ", 0) == 0) {
        int x, y, r;
        if (sscanf(line.c_str(), "pie %d %d %d", &x, &y, &r) == 3) {
            for (auto it = state_.incants.begin(); it != state_.incants.end(); ) {
                if (it->x == x && it->y == y)
                    it = state_.incants.erase(it);
                else
                    ++it;
            }
        }
        return;
    }
    if (line.rfind("pfk ", 0) == 0)
        return;
    if (line.rfind("pgt ", 0) == 0 || line.rfind("pdr ", 0) == 0)
        return;
    if (line.rfind("pex ", 0) == 0)
        return;
    if (line.rfind("enw ", 0) == 0) {
        Egg e;
        if (sscanf(line.c_str(), "enw #%d #%d %d %d",
                &e.id, &e.player_id, &e.x, &e.y) == 4)
            state_.eggs[e.id] = e;
        return;
    }
    if (line.rfind("ebo ", 0) == 0) {
        int id = atoi(line.c_str() + 4);
        state_.eggs.erase(id);
        return;
    }
    if (line.rfind("edi ", 0) == 0) {
        int id = atoi(line.c_str() + 4);
        state_.eggs.erase(id);
        return;
    }
    if (line.rfind("seg ", 0) == 0)
        state_.winner = line.substr(4);
}

void NetClient::reader_loop()
{
    char tmp[4096];
    while (running_) {
        ssize_t n = recv(sock_, tmp, sizeof(tmp) - 1, 0);
        if (n <= 0)
            break;
        tmp[n] = '\0';
        buf_ += tmp;
        size_t pos;
        while ((pos = buf_.find('\n')) != std::string::npos) {
            std::string line = buf_.substr(0, pos);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            buf_.erase(0, pos + 1);
            handle_line(line);
        }
    }
    connected_ = false;
}
