#ifndef ZAPPY_H
#define ZAPPY_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 256
#define READ_BUF 4096
#define MAX_CMD_QUEUE 10
#define FOOD_LIFE 126
#define START_FOOD 10
#define EGG_HATCH 600
#define FORK_TIME 42
#define INCANT_TIME 300
#define MOVE_TIME 7
#define INV_TIME 1
#define SPAWN_INTERVAL 20
#define WIN_LEVEL 8
#define WIN_PLAYERS 6

typedef enum e_res {
    RES_FOOD = 0,
    RES_LINEMATE,
    RES_DERAUMERE,
    RES_SIBUR,
    RES_MENDIANE,
    RES_PHIRAS,
    RES_THYSTAME,
    RES_COUNT
} res_t;

typedef enum e_orient {
    OR_NORTH = 1,
    OR_EAST = 2,
    OR_SOUTH = 3,
    OR_WEST = 4
} orient_t;

typedef enum e_client_type {
    CT_UNKNOWN = 0,
    CT_AUTH,
    CT_PLAYER,
    CT_GUI
} client_type_t;

typedef struct s_team {
    char *name;
    int slots;
    int max_clients;
    struct s_team *next;
} team_t;

typedef struct s_egg {
    int id;
    int x;
    int y;
    int64_t hatch_at;
    team_t *team;
    struct s_egg *next;
} egg_t;

typedef struct s_player {
    int id;
    int x;
    int y;
    orient_t orient;
    int level;
    int inv[RES_COUNT];
    team_t *team;
    int client_idx;
    bool alive;
    bool busy;
    int64_t next_food_tick;
    struct s_incant *incant;
    struct s_player *next;
} player_t;

typedef struct s_tile {
    int res[RES_COUNT];
    player_t *players;
    egg_t *eggs;
} tile_t;

typedef struct s_cmd {
    char *line;
    int64_t exec_at;
    struct s_cmd *next;
} cmd_t;

typedef struct s_client {
    int fd;
    client_type_t type;
    char buf[READ_BUF];
    int buf_len;
    player_t *player;
    team_t *team;
    int pending;
    cmd_t *queue;
    cmd_t *queue_tail;
} client_t;

typedef struct s_incant {
    int x;
    int y;
    int level;
    int64_t end_at;
    player_t **participants;
    int count;
    int initiator_id;
    struct s_incant *next;
} incant_t;

typedef struct s_server {
    int port;
    int width;
    int height;
    int freq;
    int sock_fd;
    tile_t **map;
    team_t *teams;
    player_t **players;
    int player_count;
    int next_player_id;
    int next_egg_id;
    client_t clients[MAX_CLIENTS];
    int client_count;
    int64_t start_us;
    int64_t next_spawn_tick;
    int res_totals[RES_COUNT];
    bool game_over;
    char *winner;
    incant_t *incants;
} server_t;

typedef struct s_args {
    int port;
    int width;
    int height;
    int clients_per_team;
    int freq;
    char **team_names;
    int team_count;
} args_t;

int parse_args(int ac, char **av, args_t *args);
void free_args(args_t *args);
void print_usage(char *prog);

server_t *server_create(args_t *args);
void server_destroy(server_t *sv);
void server_run(server_t *sv);

int64_t now_us(void);
int64_t tu_to_us(server_t *sv, int units);

tile_t *tile_at(server_t *sv, int x, int y);
void wrap_coords(server_t *sv, int *x, int *y);
int vision_size(int level);
void vision_tiles(server_t *sv, player_t *p, int *xs, int *ys, int *count);

team_t *find_team(server_t *sv, const char *name);
player_t *find_player(server_t *sv, int id);
player_t *spawn_player(server_t *sv, team_t *team, int client_idx);
void kill_player(server_t *sv, player_t *p);

void spawn_resources(server_t *sv, bool initial);
void add_resource(server_t *sv, int x, int y, res_t r);
bool take_resource(server_t *sv, player_t *p, res_t r);
bool drop_resource(server_t *sv, player_t *p, res_t r);

void gui_broadcast(server_t *sv, const char *msg);
void gui_bct(server_t *sv, int x, int y);
void gui_pnw(server_t *sv, player_t *p);
void gui_ppo(server_t *sv, player_t *p);
void gui_plv(server_t *sv, player_t *p);
void gui_pin(server_t *sv, player_t *p);
void gui_pdi(server_t *sv, int id);
void gui_pbc(server_t *sv, player_t *p, const char *msg);
void gui_pic(server_t *sv, incant_t *inc);
void gui_pie(server_t *sv, int x, int y, int result);
void gui_pfk(server_t *sv, player_t *p);
void gui_pgt(server_t *sv, player_t *p, res_t r);
void gui_pdr(server_t *sv, player_t *p, res_t r);
void gui_pex(server_t *sv, player_t *p);
void gui_enw(server_t *sv, egg_t *egg, player_t *p);
void gui_eht(server_t *sv, egg_t *egg);
void gui_ebo(server_t *sv, egg_t *egg);
void gui_edi(server_t *sv, egg_t *egg);
void gui_seg(server_t *sv, const char *team);
void gui_send_initial_state(server_t *sv, client_t *c);
void gui_smg(server_t *sv, const char *msg);
int count_team_level8(server_t *sv, team_t *team);
void notify_incant_start(server_t *sv, incant_t *inc);
bool lay_egg(server_t *sv, player_t *p);
int eject_dir(server_t *sv, player_t *from, player_t *to);
void handle_admin(server_t *sv, const char *line);

void world_tick(server_t *sv);
void move_player(server_t *sv, player_t *p, bool forward);
void turn_player(player_t *p, bool left);
void eject_players(server_t *sv, player_t *src);
bool lay_egg(server_t *sv, player_t *p);
void do_broadcast(server_t *sv, player_t *p, const char *text);
int broadcast_dir(server_t *sv, player_t *from, player_t *to);
int get_target_count(server_t *sv, res_t r);
void handle_gui_cmd(server_t *sv, client_t *c, char *line);
void handle_player_cmd(server_t *sv, client_t *c, char *line);
void process_queues(server_t *sv);
void move_forward_vec(orient_t o, int *dx, int *dy);

bool can_incant(server_t *sv, player_t *p);
void start_incant(server_t *sv, player_t *p);
void finish_incants(server_t *sv);
bool check_win(server_t *sv);

res_t parse_resource(const char *name);
const char *res_name(res_t r);
const char *res_name_fr(res_t r);

void send_fd(int fd, const char *msg);
char *build_look(server_t *sv, player_t *p);
char *build_inventory(player_t *p);

#endif
