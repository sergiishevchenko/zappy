#include "zappy.h"

static bool is_known_cmd(const char *line)
{
    char arg[256];

    if (!strcmp(line, "avance") || !strcmp(line, "droite")
        || !strcmp(line, "gauche") || !strcmp(line, "voir")
        || !strcmp(line, "inventaire") || !strcmp(line, "expulse")
        || !strcmp(line, "incantation") || !strcmp(line, "fork")
        || !strcmp(line, "connect_nbr"))
        return true;
    if (sscanf(line, "prend %255s", arg) == 1)
        return parse_resource(arg) != RES_COUNT;
    if (sscanf(line, "pose %255s", arg) == 1)
        return parse_resource(arg) != RES_COUNT;
    if (strncmp(line, "broadcast ", 10) == 0 && line[10])
        return true;
    return false;
}

static void enqueue_cmd(client_t *c, const char *line, int64_t exec_at)
{
    cmd_t *cmd = calloc(1, sizeof(cmd_t));

    if (!cmd)
        return;
    cmd->line = strdup(line);
    cmd->exec_at = exec_at;
    if (c->queue_tail) {
        c->queue_tail->next = cmd;
        c->queue_tail = cmd;
    } else {
        c->queue = c->queue_tail = cmd;
    }
    c->pending++;
}

static void free_cmd(cmd_t *cmd)
{
    if (!cmd)
        return;
    free(cmd->line);
    free(cmd);
}

static void done(client_t *c)
{
    if (c->pending > 0)
        c->pending--;
}

static void exec_cmd(server_t *sv, client_t *c, cmd_t *cmd)
{
    player_t *p = c->player;
    char *line = cmd->line;
    char arg[256];
    res_t r;

    if (!p || !p->alive) {
        free_cmd(cmd);
        return;
    }
    if (strcmp(line, "avance") == 0) {
        move_player(sv, p, true);
        send_fd(c->fd, "ok\n");
    } else if (strcmp(line, "droite") == 0) {
        turn_player(p, false);
        send_fd(c->fd, "ok\n");
    } else if (strcmp(line, "gauche") == 0) {
        turn_player(p, true);
        send_fd(c->fd, "ok\n");
    } else if (strcmp(line, "voir") == 0) {
        char *look = build_look(sv, p);
        if (look) {
            send_fd(c->fd, look);
            free(look);
        }
    } else if (strcmp(line, "inventaire") == 0) {
        char *inv = build_inventory(p);
        if (inv) {
            send_fd(c->fd, inv);
            free(inv);
        }
    } else if (sscanf(line, "prend %255s", arg) == 1) {
        r = parse_resource(arg);
        send_fd(c->fd, take_resource(sv, p, r) ? "ok\n" : "ko\n");
    } else if (sscanf(line, "pose %255s", arg) == 1) {
        r = parse_resource(arg);
        send_fd(c->fd, drop_resource(sv, p, r) ? "ok\n" : "ko\n");
    } else if (strcmp(line, "expulse") == 0) {
        tile_t *t = tile_at(sv, p->x, p->y);
        bool has_other = false;

        for (player_t *pl = t->players; pl; pl = pl->next)
            if (pl != p && pl->alive)
                has_other = true;
        if (has_other) {
            eject_players(sv, p);
            send_fd(c->fd, "ok\n");
        } else
            send_fd(c->fd, "ko\n");
    } else if (strncmp(line, "broadcast ", 10) == 0) {
        do_broadcast(sv, p, line + 10);
        send_fd(c->fd, "ok\n");
    } else if (strcmp(line, "incantation") == 0) {
        if (can_incant(sv, p))
            start_incant(sv, p);
        else
            send_fd(c->fd, "ko\n");
    } else if (strcmp(line, "fork") == 0) {
        send_fd(c->fd, lay_egg(sv, p) ? "ok\n" : "ko\n");
    } else if (strcmp(line, "connect_nbr") == 0) {
        char msg[32];

        snprintf(msg, sizeof(msg), "%d\n", p->team->slots);
        send_fd(c->fd, msg);
    } else
        send_fd(c->fd, "ko\n");
    done(c);
    free_cmd(cmd);
}

void process_queues(server_t *sv)
{
    int64_t n = now_us();

    for (int i = 0; i < sv->client_count; i++) {
        client_t *c = &sv->clients[i];

        if (c->type != CT_PLAYER || c->fd < 0)
            continue;
        while (c->queue && n >= c->queue->exec_at) {
            cmd_t *cmd = c->queue;
            c->queue = cmd->next;
            if (!c->queue)
                c->queue_tail = NULL;
            exec_cmd(sv, c, cmd);
        }
    }
}

static int cmd_cost(const char *line)
{
    if (strcmp(line, "connect_nbr") == 0)
        return 0;
    if (strcmp(line, "inventaire") == 0)
        return INV_TIME;
    if (strcmp(line, "fork") == 0)
        return FORK_TIME;
    if (strcmp(line, "incantation") == 0)
        return INCANT_TIME;
    return MOVE_TIME;
}

void handle_player_cmd(server_t *sv, client_t *c, char *line)
{
    int64_t exec_at;
    int cost;

    if (!c->player || !c->player->alive)
        return;
    if (!is_known_cmd(line)) {
        send_fd(c->fd, "ko\n");
        return;
    }
    if (c->pending >= MAX_CMD_QUEUE)
        return;
    if (c->player->busy && strcmp(line, "incantation") != 0)
        return;
    cost = cmd_cost(line);
    if (c->queue_tail)
        exec_at = c->queue_tail->exec_at + tu_to_us(sv, cost);
    else
        exec_at = now_us() + tu_to_us(sv, cost);
    enqueue_cmd(c, line, exec_at);
    if (cost == 0)
        process_queues(sv);
}
