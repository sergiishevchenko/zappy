#include "zappy.h"

void move_forward_vec(orient_t o, int *dx, int *dy);

static void unlink_player(tile_t *t, player_t *p)
{
    player_t **cur = &t->players;

    while (*cur) {
        if (*cur == p) {
            *cur = p->next;
            p->next = NULL;
            return;
        }
        cur = &(*cur)->next;
    }
}

static void link_player(tile_t *t, player_t *p)
{
    p->next = t->players;
    t->players = p;
}

player_t *spawn_player(server_t *sv, team_t *team, int client_idx)
{
    player_t *p = calloc(1, sizeof(player_t));
    tile_t *t;

    if (!p)
        return NULL;
    p->id = sv->next_player_id++;
    p->x = rand() % sv->width;
    p->y = rand() % sv->height;
    p->orient = (orient_t)(1 + rand() % 4);
    p->level = 1;
    p->inv[RES_FOOD] = START_FOOD;
    p->team = team;
    p->client_idx = client_idx;
    p->alive = true;
    p->next_food_tick = now_us() + tu_to_us(sv, FOOD_LIFE);
    sv->players = realloc(sv->players, sizeof(player_t *) * (sv->player_count + 1));
    sv->players[sv->player_count++] = p;
    t = tile_at(sv, p->x, p->y);
    link_player(t, p);
    gui_pnw(sv, p);
    return p;
}

void kill_player(server_t *sv, player_t *p)
{
    client_t *c;

    if (!p || !p->alive)
        return;
    p->alive = false;
    if (p->team)
        p->team->slots++;
    unlink_player(tile_at(sv, p->x, p->y), p);
    gui_pdi(sv, p->id);
    if (p->client_idx >= 0 && p->client_idx < MAX_CLIENTS) {
        c = &sv->clients[p->client_idx];
        send_fd(c->fd, "mort\n");
        c->player = NULL;
    }
}

void add_resource(server_t *sv, int x, int y, res_t r)
{
    tile_t *t = tile_at(sv, x, y);

    t->res[r]++;
    sv->res_totals[r]++;
}

bool take_resource(server_t *sv, player_t *p, res_t r)
{
    tile_t *t = tile_at(sv, p->x, p->y);

    if (r >= RES_COUNT || t->res[r] <= 0)
        return false;
    t->res[r]--;
    sv->res_totals[r]--;
    p->inv[r]++;
    gui_pgt(sv, p, r);
    gui_bct(sv, p->x, p->y);
    return true;
}

bool drop_resource(server_t *sv, player_t *p, res_t r)
{
    tile_t *t = tile_at(sv, p->x, p->y);

    if (r >= RES_COUNT || p->inv[r] <= 0)
        return false;
    p->inv[r]--;
    t->res[r]++;
    sv->res_totals[r]++;
    gui_pdr(sv, p, r);
    gui_bct(sv, p->x, p->y);
    return true;
}

static void spawn_one(server_t *sv, res_t r)
{
    int x = rand() % sv->width;
    int y = rand() % sv->height;

    add_resource(sv, x, y, r);
    gui_bct(sv, x, y);
}

void spawn_resources(server_t *sv, bool initial)
{
    for (int r = 0; r < RES_COUNT; r++) {
        int target = get_target_count(sv, (res_t)r);
        while (sv->res_totals[r] < target) {
            spawn_one(sv, (res_t)r);
            if (!initial && sv->res_totals[r] >= target)
                break;
        }
    }
    if (initial) {
        for (int r = 0; r < RES_COUNT; r++) {
            if (sv->res_totals[r] < 1)
                spawn_one(sv, (res_t)r);
        }
    }
}

static void tick_food(server_t *sv)
{
    int64_t n = now_us();

    for (int i = 0; i < sv->player_count; i++) {
        player_t *p = sv->players[i];

        if (!p || !p->alive)
            continue;
        while (p->alive && n >= p->next_food_tick) {
            if (p->inv[RES_FOOD] <= 0) {
                kill_player(sv, p);
                break;
            }
            p->inv[RES_FOOD]--;
            p->next_food_tick += tu_to_us(sv, FOOD_LIFE);
        }
    }
}

static void tick_eggs(server_t *sv)
{
    int64_t n = now_us();

    for (int y = 0; y < sv->height; y++) {
        for (int x = 0; x < sv->width; x++) {
            egg_t *egg = sv->map[y][x].eggs;
            egg_t *prev = NULL;

            while (egg) {
                if (n >= egg->hatch_at) {
                    egg->team->slots++;
                    gui_eht(sv, egg);
                    gui_ebo(sv, egg);
                    gui_edi(sv, egg);
                    if (prev)
                        prev->next = egg->next;
                    else
                        sv->map[y][x].eggs = egg->next;
                    free(egg);
                    egg = prev ? prev->next : sv->map[y][x].eggs;
                } else {
                    prev = egg;
                    egg = egg->next;
                }
            }
        }
    }
}

void world_tick(server_t *sv)
{
    int64_t elapsed = (now_us() - sv->start_us) / tu_to_us(sv, 1);

    tick_food(sv);
    tick_eggs(sv);
    finish_incants(sv);
    if (elapsed >= sv->next_spawn_tick) {
        spawn_resources(sv, false);
        sv->next_spawn_tick = elapsed + SPAWN_INTERVAL;
    }
}

void move_player(server_t *sv, player_t *p, bool forward)
{
    int dx = 0;
    int dy = 0;
    tile_t *old;
    tile_t *nw;

    move_forward_vec(p->orient, &dx, &dy);
    if (!forward) {
        dx = -dx;
        dy = -dy;
    }
    old = tile_at(sv, p->x, p->y);
    unlink_player(old, p);
    p->x += dx;
    p->y += dy;
    wrap_coords(sv, &p->x, &p->y);
    nw = tile_at(sv, p->x, p->y);
    link_player(nw, p);
    gui_ppo(sv, p);
}

void turn_player(player_t *p, bool left)
{
    if (left) {
        p->orient = (orient_t)(p->orient == OR_NORTH ? OR_WEST : p->orient - 1);
    } else {
        p->orient = (orient_t)(p->orient == OR_WEST ? OR_NORTH : p->orient + 1);
    }
}

void eject_players(server_t *sv, player_t *src)
{
    tile_t *t = tile_at(sv, src->x, src->y);
    int dx = 0;
    int dy = 0;
    player_t *list = t->players;
    int k;

    move_forward_vec(src->orient, &dx, &dy);
    while (list) {
        player_t *next = list->next;
        if (list != src && list->alive) {
            unlink_player(t, list);
            list->x += dx;
            list->y += dy;
            wrap_coords(sv, &list->x, &list->y);
            link_player(tile_at(sv, list->x, list->y), list);
            gui_ppo(sv, list);
            if (list->client_idx >= 0) {
                char msg[64];
                k = eject_dir(sv, src, list);
                snprintf(msg, sizeof(msg), "deplacement %d\n", k);
                send_fd(sv->clients[list->client_idx].fd, msg);
            }
            gui_pex(sv, src);
        }
        list = next;
    }
}

bool lay_egg(server_t *sv, player_t *p)
{
    egg_t *egg = calloc(1, sizeof(egg_t));
    tile_t *t = tile_at(sv, p->x, p->y);
    int eggs = 0;

    if (!egg)
        return false;
    for (int y = 0; y < sv->height; y++)
        for (int x = 0; x < sv->width; x++)
            for (egg_t *e = sv->map[y][x].eggs; e; e = e->next)
                if (e->team == p->team)
                    eggs++;
    if (p->team->slots > 0 || eggs >= p->team->max_clients) {
        free(egg);
        return false;
    }
    egg->id = sv->next_egg_id++;
    egg->x = p->x;
    egg->y = p->y;
    egg->team = p->team;
    egg->hatch_at = now_us() + tu_to_us(sv, EGG_HATCH);
    egg->next = t->eggs;
    t->eggs = egg;
    gui_pfk(sv, p);
    gui_enw(sv, egg, p);
    return true;
}

void do_broadcast(server_t *sv, player_t *p, const char *text)
{
    char msg[1024];

    gui_pbc(sv, p, text);
    for (int i = 0; i < sv->player_count; i++) {
        player_t *other = sv->players[i];
        int dir;

        if (!other || !other->alive || other == p)
            continue;
        if (other->client_idx < 0)
            continue;
        dir = broadcast_dir(sv, p, other);
        snprintf(msg, sizeof(msg), "message %d,%s\n", dir, text);
        send_fd(sv->clients[other->client_idx].fd, msg);
    }
}

