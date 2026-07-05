#include "zappy.h"

static const double DENSITIES[RES_COUNT] = {
    0.5, 0.3, 0.15, 0.1, 0.1, 0.08, 0.05
};

static const int INCANT_PLAYERS[7] = {1, 2, 2, 4, 4, 6, 6};
static const int INCANT_STONES[7][6] = {
    {1, 0, 0, 0, 0, 0},
    {1, 1, 1, 0, 0, 0},
    {2, 0, 1, 0, 2, 0},
    {1, 1, 2, 0, 1, 0},
    {1, 2, 1, 3, 0, 0},
    {1, 2, 3, 0, 1, 0},
    {2, 2, 2, 2, 2, 1}
};

void print_usage(char *prog)
{
    fprintf(stderr,
        "USAGE: %s -p <port> -x <width> -y <height> "
        "-n <team> [<team> ...] -c <nb> -t <t>\n",
        prog);
}

static bool is_flag(const char *s)
{
    return s && s[0] == '-' && s[1] && !s[2];
}

int parse_args(int ac, char **av, args_t *args)
{
    memset(args, 0, sizeof(*args));
    args->freq = 100;
    for (int i = 1; i < ac; i++) {
        if (!is_flag(av[i]))
            continue;
        if (strcmp(av[i], "-p") == 0 && i + 1 < ac)
            args->port = atoi(av[++i]);
        else if (strcmp(av[i], "-x") == 0 && i + 1 < ac)
            args->width = atoi(av[++i]);
        else if (strcmp(av[i], "-y") == 0 && i + 1 < ac)
            args->height = atoi(av[++i]);
        else if (strcmp(av[i], "-c") == 0 && i + 1 < ac)
            args->clients_per_team = atoi(av[++i]);
        else if (strcmp(av[i], "-t") == 0 && i + 1 < ac)
            args->freq = atoi(av[++i]);
        else if (strcmp(av[i], "-n") == 0) {
            while (i + 1 < ac && !is_flag(av[i + 1])) {
                args->team_names = realloc(args->team_names,
                    sizeof(char *) * (args->team_count + 1));
                args->team_names[args->team_count++] = strdup(av[++i]);
            }
        } else
            return -1;
    }
    if (args->port <= 0 || args->width <= 0 || args->height <= 0
        || args->clients_per_team <= 0 || args->freq <= 0
        || args->team_count <= 0)
        return -1;
    return 0;
}

void free_args(args_t *args)
{
    for (int i = 0; i < args->team_count; i++)
        free(args->team_names[i]);
    free(args->team_names);
}

int64_t now_us(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (int64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

int64_t tu_to_us(server_t *sv, int units)
{
    return (int64_t)units * 1000000 / sv->freq;
}

void send_fd(int fd, const char *msg)
{
    size_t len;
    ssize_t sent;

    if (fd < 0 || !msg)
        return;
    len = strlen(msg);
    while (len > 0) {
        sent = send(fd, msg, len, 0);
        if (sent <= 0)
            return;
        msg += sent;
        len -= (size_t)sent;
    }
}

res_t parse_resource(const char *name)
{
    static const struct { const char *n; res_t r; } map[] = {
        {"food", RES_FOOD}, {"nourriture", RES_FOOD},
        {"linemate", RES_LINEMATE}, {"deraumere", RES_DERAUMERE},
        {"sibur", RES_SIBUR}, {"mendiane", RES_MENDIANE},
        {"phiras", RES_PHIRAS}, {"thystame", RES_THYSTAME},
        {NULL, RES_COUNT}
    };

    for (int i = 0; map[i].n; i++)
        if (strcasecmp(name, map[i].n) == 0)
            return map[i].r;
    return RES_COUNT;
}

const char *res_name(res_t r)
{
    static const char *names[] = {
        "food", "linemate", "deraumere", "sibur",
        "mendiane", "phiras", "thystame"
    };
    if (r < 0 || r >= RES_COUNT)
        return "unknown";
    return names[r];
}

const char *res_name_fr(res_t r)
{
    if (r == RES_FOOD)
        return "nourriture";
    return res_name(r);
}

int get_target_count(server_t *sv, res_t r)
{
    return (int)(sv->width * sv->height * DENSITIES[r]);
}

int vision_size(int level)
{
    int total = 0;

    for (int i = 0; i <= level; i++)
        total += 2 * i + 1;
    return total;
}

tile_t *tile_at(server_t *sv, int x, int y)
{
    int wx = x;
    int wy = y;

    wrap_coords(sv, &wx, &wy);
    return &sv->map[wy][wx];
}

void wrap_coords(server_t *sv, int *x, int *y)
{
    while (*x < 0)
        *x += sv->width;
    while (*y < 0)
        *y += sv->height;
    *x %= sv->width;
    *y %= sv->height;
}

team_t *find_team(server_t *sv, const char *name)
{
    for (team_t *t = sv->teams; t; t = t->next)
        if (strcmp(t->name, name) == 0)
            return t;
    return NULL;
}

player_t *find_player(server_t *sv, int id)
{
    for (int i = 0; i < sv->player_count; i++)
        if (sv->players[i] && sv->players[i]->id == id)
            return sv->players[i];
    return NULL;
}

void move_forward_vec(orient_t o, int *dx, int *dy)
{
    *dx = 0;
    *dy = 0;
    if (o == OR_NORTH)
        *dy = -1;
    else if (o == OR_SOUTH)
        *dy = 1;
    else if (o == OR_EAST)
        *dx = 1;
    else if (o == OR_WEST)
        *dx = -1;
}

static void rotate_vec(int *dx, int *dy, bool left)
{
    int ndx;
    int ndy;

    if (left) {
        ndx = -*dy;
        ndy = *dx;
    } else {
        ndx = *dy;
        ndy = -*dx;
    }
    *dx = ndx;
    *dy = ndy;
}

void vision_tiles(server_t *sv, player_t *p, int *xs, int *ys, int *count)
{
    int fdx = 0;
    int fdy = 0;
    int rdx = 0;
    int rdy = 0;
    int idx = 0;

    move_forward_vec(p->orient, &fdx, &fdy);
    rdx = fdx;
    rdy = fdy;
    rotate_vec(&rdx, &rdy, false);
    for (int row = 0; row <= p->level; row++) {
        int width = 2 * row + 1;
        int cx = p->x + fdx * (row + 1) - rdx * row;
        int cy = p->y + fdy * (row + 1) - rdy * row;

        for (int col = 0; col < width; col++) {
            xs[idx] = cx + rdx * col;
            ys[idx] = cy + rdy * col;
            wrap_coords(sv, &xs[idx], &ys[idx]);
            idx++;
        }
    }
    *count = idx;
}

static void tile_content_str(server_t *sv, int x, int y, char *out, size_t sz)
{
    tile_t *t = tile_at(sv, x, y);
    size_t pos = 0;
    bool first = true;

    out[0] = '\0';
    for (int r = 0; r < RES_COUNT; r++) {
        for (int i = 0; i < t->res[r]; i++) {
            if (!first && pos + 1 < sz) {
                out[pos++] = ' ';
                out[pos] = '\0';
            }
            pos += snprintf(out + pos, sz - pos, "%s", res_name_fr((res_t)r));
            first = false;
        }
    }
    for (player_t *pl = t->players; pl; pl = pl->next) {
        if (!first && pos + 1 < sz) {
            out[pos++] = ' ';
            out[pos] = '\0';
        }
        pos += snprintf(out + pos, sz - pos, "player");
        first = false;
    }
}

char *build_look(server_t *sv, player_t *p)
{
    int xs[256];
    int ys[256];
    int count = 0;
    char tile[512];
    size_t len;
    char *out = malloc(4096);

    if (!out)
        return NULL;
    strcpy(out, "[");
    len = 1;
    vision_tiles(sv, p, xs, ys, &count);
    for (int i = 0; i < count; i++) {
        tile_content_str(sv, xs[i], ys[i], tile, sizeof(tile));
        if (i > 0)
            len += snprintf(out + len, 4096 - len, ", ");
        len += snprintf(out + len, 4096 - len, "%s", tile);
    }
    snprintf(out + len, 4096 - len, "]\n");
    return out;
}

char *build_inventory(player_t *p)
{
    char *out = malloc(512);

    if (!out)
        return NULL;
    snprintf(out, 512,
        "[nourriture %d, linemate %d, deraumere %d, sibur %d, "
        "mendiane %d, phiras %d, thystame %d]\n",
        p->inv[RES_FOOD], p->inv[RES_LINEMATE], p->inv[RES_DERAUMERE],
        p->inv[RES_SIBUR], p->inv[RES_MENDIANE], p->inv[RES_PHIRAS],
        p->inv[RES_THYSTAME]);
    return out;
}

bool can_incant(server_t *sv, player_t *p)
{
    int lvl = p->level;
    tile_t *t;
    int need_p;
    int on_tile = 0;

    if (lvl < 1 || lvl > 7 || p->busy)
        return false;
    t = tile_at(sv, p->x, p->y);
    need_p = INCANT_PLAYERS[lvl - 1];
    for (int i = 1; i < RES_COUNT; i++)
        if (t->res[i] < INCANT_STONES[lvl - 1][i - 1])
            return false;
    for (player_t *pl = t->players; pl; pl = pl->next)
        if (pl->alive && pl->level == lvl && !pl->busy)
            on_tile++;
    return on_tile >= need_p;
}

int count_team_level8(server_t *sv, team_t *team)
{
    int n = 0;

    for (int i = 0; i < sv->player_count; i++) {
        player_t *p = sv->players[i];
        if (p && p->alive && p->team == team && p->level >= WIN_LEVEL)
            n++;
    }
    return n;
}

void notify_incant_start(server_t *sv, incant_t *inc)
{
    char msg[64];

    for (int i = 0; i < inc->count; i++) {
        player_t *pl = inc->participants[i];
        client_t *c;

        if (!pl || pl->client_idx < 0)
            continue;
        c = &sv->clients[pl->client_idx];
        send_fd(c->fd, "elevation en cours\n");
        snprintf(msg, sizeof(msg), "niveau actuel : %d\n", pl->level);
        send_fd(c->fd, msg);
    }
}

static bool incant_still_valid(server_t *sv, incant_t *inc)
{
    tile_t *t = tile_at(sv, inc->x, inc->y);
    int need = INCANT_PLAYERS[inc->level - 1];
    int on = 0;

    for (int i = 0; i < inc->count; i++) {
        player_t *pl = inc->participants[i];
        if (pl && pl->alive && pl->level == inc->level
            && pl->x == inc->x && pl->y == inc->y)
            on++;
    }
    if (on < need)
        return false;
    for (int i = 1; i < RES_COUNT; i++)
        if (t->res[i] < INCANT_STONES[inc->level - 1][i - 1])
            return false;
    return true;
}

void start_incant(server_t *sv, player_t *p)
{
    tile_t *t = tile_at(sv, p->x, p->y);
    incant_t *inc = calloc(1, sizeof(incant_t));
    int need = INCANT_PLAYERS[p->level - 1];
    int n = 0;

    if (!inc)
        return;
    inc->x = p->x;
    inc->y = p->y;
    inc->level = p->level;
    inc->initiator_id = p->id;
    inc->end_at = now_us() + tu_to_us(sv, INCANT_TIME);
    inc->participants = calloc(need, sizeof(player_t *));
    for (player_t *pl = t->players; pl && n < need; pl = pl->next) {
        if (!pl->alive || pl->level != p->level || pl->busy)
            continue;
        inc->participants[n++] = pl;
        pl->busy = true;
        pl->incant = inc;
    }
    if (n < need) {
        for (int i = 0; i < n; i++) {
            inc->participants[i]->busy = false;
            inc->participants[i]->incant = NULL;
        }
        free(inc->participants);
        free(inc);
        return;
    }
    inc->count = n;
    inc->next = sv->incants;
    sv->incants = inc;
    gui_pic(sv, inc);
    notify_incant_start(sv, inc);
}

void finish_incants(server_t *sv)
{
    incant_t *inc = sv->incants;
    incant_t *prev = NULL;

    while (inc) {
        if (now_us() < inc->end_at) {
            prev = inc;
            inc = inc->next;
            continue;
        }
        player_t *init = find_player(sv, inc->initiator_id);
        bool ok = init && init->alive && incant_still_valid(sv, inc);
        if (ok) {
            tile_t *t = tile_at(sv, inc->x, inc->y);
            int lvl = inc->level;
            for (int i = 1; i < RES_COUNT; i++)
                t->res[i] -= INCANT_STONES[lvl - 1][i - 1];
            gui_bct(sv, inc->x, inc->y);
            for (int i = 0; i < inc->count; i++) {
                player_t *pl = inc->participants[i];
                if (pl && pl->alive) {
                    pl->level++;
                    gui_plv(sv, pl);
                }
                if (pl) {
                    pl->busy = false;
                    pl->incant = NULL;
                }
            }
            check_win(sv);
        } else {
            for (int i = 0; i < inc->count; i++)
                if (inc->participants[i]) {
                    inc->participants[i]->busy = false;
                    inc->participants[i]->incant = NULL;
                }
        }
        gui_pie(sv, inc->x, inc->y, ok ? 1 : 0);
        if (prev)
            prev->next = inc->next;
        else
            sv->incants = inc->next;
        free(inc->participants);
        free(inc);
        inc = prev ? prev->next : sv->incants;
    }
}

bool check_win(server_t *sv)
{
    if (sv->game_over)
        return true;
    for (team_t *t = sv->teams; t; t = t->next) {
        if (count_team_level8(sv, t) >= WIN_PLAYERS) {
            sv->game_over = true;
            sv->winner = t->name;
            gui_seg(sv, t->name);
            return true;
        }
    }
    return false;
}

static int torus_dist(server_t *sv, int x1, int y1, int x2, int y2)
{
    int dx = x2 - x1;
    int dy = y2 - y1;

    if (dx > sv->width / 2)
        dx -= sv->width;
    else if (dx < -(sv->width / 2))
        dx += sv->width;
    if (dy > sv->height / 2)
        dy -= sv->height;
    else if (dy < -(sv->height / 2))
        dy += sv->height;
    return abs(dx) + abs(dy);
}

int eject_dir(server_t *sv, player_t *from, player_t *to)
{
    return broadcast_dir(sv, from, to);
}

int broadcast_dir(server_t *sv, player_t *from, player_t *to)
{
    int best_k = 0;
    int best_dist = 999999;
    int fdx = 0;
    int fdy = 0;
    int rdx = 0;
    int rdy = 0;

    if (from->x == to->x && from->y == to->y)
        return 0;
    move_forward_vec(to->orient, &fdx, &fdy);
    rdx = fdx;
    rdy = fdy;
    rotate_vec(&rdx, &rdy, false);
    for (int k = 1; k <= 8; k++) {
        int ox = 0;
        int oy = 0;
        if (k == 1) { ox = fdx; oy = fdy; }
        else if (k == 2) { ox = rdx; oy = rdy; }
        else if (k == 3) { ox = fdx + rdx; oy = fdy + rdy; }
        else if (k == 4) { ox = rdx - fdx; oy = rdy - fdy; }
        else if (k == 5) { ox = -fdx; oy = -fdy; }
        else if (k == 6) { ox = -rdx; oy = -rdy; }
        else if (k == 7) { ox = -fdx - rdx; oy = -fdy - rdy; }
        else { ox = -rdx + fdx; oy = -rdy + fdy; }
        int tx = to->x + ox;
        int ty = to->y + oy;
        wrap_coords(sv, &tx, &ty);
        if (tx == from->x && ty == from->y) {
            int dist = torus_dist(sv, to->x, to->y, from->x, from->y);
            if (dist < best_dist) {
                best_dist = dist;
                best_k = k;
            }
        }
    }
    return best_k;
}
