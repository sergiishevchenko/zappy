#include "zappy.h"

void gui_broadcast(server_t *sv, const char *msg)
{
    for (int i = 0; i < sv->client_count; i++)
        if (sv->clients[i].type == CT_GUI && sv->clients[i].fd >= 0)
            send_fd(sv->clients[i].fd, msg);
}

void gui_bct(server_t *sv, int x, int y)
{
    char msg[256];
    tile_t *t = tile_at(sv, x, y);

    snprintf(msg, sizeof(msg),
        "bct %d %d %d %d %d %d %d %d %d\n",
        x, y,
        t->res[RES_FOOD], t->res[RES_LINEMATE], t->res[RES_DERAUMERE],
        t->res[RES_SIBUR], t->res[RES_MENDIANE], t->res[RES_PHIRAS],
        t->res[RES_THYSTAME]);
    gui_broadcast(sv, msg);
}

void gui_pnw(server_t *sv, player_t *p)
{
    char msg[256];

    snprintf(msg, sizeof(msg), "pnw #%d %d %d %d %d %s\n",
        p->id, p->x, p->y, p->orient, p->level, p->team->name);
    gui_broadcast(sv, msg);
}

void gui_ppo(server_t *sv, player_t *p)
{
    char msg[128];

    snprintf(msg, sizeof(msg), "ppo #%d %d %d %d\n",
        p->id, p->x, p->y, p->orient);
    gui_broadcast(sv, msg);
}

void gui_plv(server_t *sv, player_t *p)
{
    char msg[64];

    snprintf(msg, sizeof(msg), "plv #%d %d\n", p->id, p->level);
    gui_broadcast(sv, msg);
}

void gui_pin(server_t *sv, player_t *p)
{
    char msg[256];

    snprintf(msg, sizeof(msg),
        "pin #%d %d %d %d %d %d %d %d %d %d\n",
        p->id, p->x, p->y,
        p->inv[RES_FOOD], p->inv[RES_LINEMATE], p->inv[RES_DERAUMERE],
        p->inv[RES_SIBUR], p->inv[RES_MENDIANE], p->inv[RES_PHIRAS],
        p->inv[RES_THYSTAME]);
    gui_broadcast(sv, msg);
}

void gui_pdi(server_t *sv, int id)
{
    char msg[32];

    snprintf(msg, sizeof(msg), "pdi #%d\n", id);
    gui_broadcast(sv, msg);
}

void gui_pbc(server_t *sv, player_t *p, const char *msg_text)
{
    char msg[512];

    snprintf(msg, sizeof(msg), "pbc #%d %s\n", p->id, msg_text);
    gui_broadcast(sv, msg);
}

void gui_pic(server_t *sv, incant_t *inc)
{
    char msg[512];
    int pos;

    pos = snprintf(msg, sizeof(msg), "pic %d %d %d",
        inc->x, inc->y, inc->level);
    for (int i = 0; i < inc->count; i++)
        pos += snprintf(msg + pos, sizeof(msg) - pos, " #%d",
            inc->participants[i]->id);
    snprintf(msg + pos, sizeof(msg) - pos, "\n");
    gui_broadcast(sv, msg);
}

void gui_pie(server_t *sv, int x, int y, int result)
{
    char msg[64];

    snprintf(msg, sizeof(msg), "pie %d %d %d\n", x, y, result);
    gui_broadcast(sv, msg);
}

void gui_pfk(server_t *sv, player_t *p)
{
    char msg[32];

    snprintf(msg, sizeof(msg), "pfk #%d\n", p->id);
    gui_broadcast(sv, msg);
}

void gui_pgt(server_t *sv, player_t *p, res_t r)
{
    char msg[64];

    snprintf(msg, sizeof(msg), "pgt #%d %d\n", p->id, r);
    gui_broadcast(sv, msg);
}

void gui_pdr(server_t *sv, player_t *p, res_t r)
{
    char msg[64];

    snprintf(msg, sizeof(msg), "pdr #%d %d\n", p->id, r);
    gui_broadcast(sv, msg);
}

void gui_pex(server_t *sv, player_t *p)
{
    char msg[32];

    snprintf(msg, sizeof(msg), "pex #%d\n", p->id);
    gui_broadcast(sv, msg);
}

void gui_enw(server_t *sv, egg_t *egg, player_t *p)
{
    char msg[64];

    snprintf(msg, sizeof(msg), "enw #%d #%d %d %d\n",
        egg->id, p->id, egg->x, egg->y);
    gui_broadcast(sv, msg);
}

void gui_eht(server_t *sv, egg_t *egg)
{
    char msg[32];

    (void)egg;
    snprintf(msg, sizeof(msg), "eht #%d\n", egg->id);
    gui_broadcast(sv, msg);
}

void gui_ebo(server_t *sv, egg_t *egg)
{
    char msg[32];

    snprintf(msg, sizeof(msg), "ebo #%d\n", egg->id);
    gui_broadcast(sv, msg);
}

void gui_edi(server_t *sv, egg_t *egg)
{
    char msg[32];

    snprintf(msg, sizeof(msg), "edi #%d\n", egg->id);
    gui_broadcast(sv, msg);
}

void gui_seg(server_t *sv, const char *team)
{
    char msg[128];

    snprintf(msg, sizeof(msg), "seg %s\n", team);
    gui_broadcast(sv, msg);
}

static void gui_mgt(server_t *sv, client_t *c)
{
    char msg[128];

    (void)sv;

    snprintf(msg, sizeof(msg),
        "mgt %d %d %d %d %d %d %d %d %d %d %d\n",
        MOVE_TIME, MOVE_TIME, MOVE_TIME, MOVE_TIME, INV_TIME,
        MOVE_TIME, MOVE_TIME, INCANT_TIME, FORK_TIME, 0, FOOD_LIFE);
    send_fd(c->fd, msg);
}

static void gui_msz(server_t *sv, client_t *c)
{
    char msg[64];

    snprintf(msg, sizeof(msg), "msz %d %d\n", sv->width, sv->height);
    send_fd(c->fd, msg);
}

static void gui_mct(server_t *sv, client_t *c)
{
    for (int y = 0; y < sv->height; y++)
        for (int x = 0; x < sv->width; x++) {
            char msg[256];
            tile_t *t = &sv->map[y][x];

            snprintf(msg, sizeof(msg),
                "bct %d %d %d %d %d %d %d %d %d\n",
                x, y,
                t->res[RES_FOOD], t->res[RES_LINEMATE], t->res[RES_DERAUMERE],
                t->res[RES_SIBUR], t->res[RES_MENDIANE], t->res[RES_PHIRAS],
                t->res[RES_THYSTAME]);
            send_fd(c->fd, msg);
        }
}

static void gui_tna(server_t *sv, client_t *c)
{
    for (team_t *t = sv->teams; t; t = t->next) {
        char msg[128];

        snprintf(msg, sizeof(msg), "tna %s\n", t->name);
        send_fd(c->fd, msg);
    }
}

static void gui_sgt(server_t *sv, client_t *c)
{
    char msg[32];

    snprintf(msg, sizeof(msg), "sgt %d\n", sv->freq);
    send_fd(c->fd, msg);
}

void gui_send_initial_state(server_t *sv, client_t *c)
{
    char msg[256];

    gui_msz(sv, c);
    gui_tna(sv, c);
    gui_sgt(sv, c);
    gui_mct(sv, c);
    for (int i = 0; i < sv->player_count; i++) {
        player_t *p = sv->players[i];

        if (!p || !p->alive)
            continue;
        snprintf(msg, sizeof(msg), "pnw #%d %d %d %d %d %s\n",
            p->id, p->x, p->y, p->orient, p->level, p->team->name);
        send_fd(c->fd, msg);
    }
    for (int y = 0; y < sv->height; y++) {
        for (int x = 0; x < sv->width; x++) {
            for (egg_t *egg = sv->map[y][x].eggs; egg; egg = egg->next) {
                snprintf(msg, sizeof(msg), "enw #%d #0 %d %d\n",
                    egg->id, egg->x, egg->y);
                send_fd(c->fd, msg);
            }
        }
    }
}

void handle_gui_cmd(server_t *sv, client_t *c, char *line)
{
    int a = 0;
    int b = 0;
    int n = 0;
    player_t *p;

    if (sscanf(line, "msz%n", &n) == 0 && line[n] == '\0') {
        gui_msz(sv, c);
        return;
    }
    if (sscanf(line, "mct%n", &n) == 0 && line[n] == '\0') {
        gui_mct(sv, c);
        return;
    }
    if (sscanf(line, "tna%n", &n) == 0 && line[n] == '\0') {
        gui_tna(sv, c);
        return;
    }
    if (sscanf(line, "sgt%n", &n) == 0 && line[n] == '\0') {
        gui_sgt(sv, c);
        return;
    }
    if (sscanf(line, "mgt%n", &n) == 0 && line[n] == '\0') {
        gui_mgt(sv, c);
        return;
    }
    if (sscanf(line, "sst %d%n", &a, &n) == 1 && line[n] == '\0') {
        char msg[32];
        sv->freq = a;
        snprintf(msg, sizeof(msg), "sst %d\n", sv->freq);
        gui_broadcast(sv, msg);
        return;
    }
    if (sscanf(line, "bct %d %d%n", &a, &b, &n) == 2 && line[n] == '\0') {
        char msg[256];
        tile_t *t = tile_at(sv, a, b);
        snprintf(msg, sizeof(msg),
            "bct %d %d %d %d %d %d %d %d %d\n",
            a, b,
            t->res[RES_FOOD], t->res[RES_LINEMATE], t->res[RES_DERAUMERE],
            t->res[RES_SIBUR], t->res[RES_MENDIANE], t->res[RES_PHIRAS],
            t->res[RES_THYSTAME]);
        send_fd(c->fd, msg);
        return;
    }
    if (sscanf(line, "ppo #%d%n", &n, &a) == 1 && line[a] == '\0') {
        p = find_player(sv, n);
        if (p) {
            char msg[128];
            snprintf(msg, sizeof(msg), "ppo #%d %d %d %d\n",
                p->id, p->x, p->y, p->orient);
            send_fd(c->fd, msg);
        } else
            send_fd(c->fd, "sbp\n");
        return;
    }
    if (sscanf(line, "plv #%d%n", &n, &a) == 1 && line[a] == '\0') {
        p = find_player(sv, n);
        if (p) {
            char msg[32];
            snprintf(msg, sizeof(msg), "plv #%d %d\n", p->id, p->level);
            send_fd(c->fd, msg);
        } else
            send_fd(c->fd, "sbp\n");
        return;
    }
    if (sscanf(line, "pin #%d%n", &n, &a) == 1 && line[a] == '\0') {
        p = find_player(sv, n);
        if (p) {
            char msg[256];
            snprintf(msg, sizeof(msg),
                "pin #%d %d %d %d %d %d %d %d %d %d\n",
                p->id, p->x, p->y,
                p->inv[RES_FOOD], p->inv[RES_LINEMATE], p->inv[RES_DERAUMERE],
                p->inv[RES_SIBUR], p->inv[RES_MENDIANE], p->inv[RES_PHIRAS],
                p->inv[RES_THYSTAME]);
            send_fd(c->fd, msg);
        } else
            send_fd(c->fd, "sbp\n");
        return;
    }
    send_fd(c->fd, "suc\n");
}
