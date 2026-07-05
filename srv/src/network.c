#include "zappy.h"

void handle_player_cmd(server_t *sv, client_t *c, char *line);
void handle_gui_cmd(server_t *sv, client_t *c, char *line);
void process_queues(server_t *sv);

static int add_client(server_t *sv, int fd)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (sv->clients[i].fd < 0) {
            sv->clients[i].fd = fd;
            sv->clients[i].type = CT_AUTH;
            sv->clients[i].buf_len = 0;
            sv->clients[i].player = NULL;
            sv->clients[i].team = NULL;
            sv->clients[i].pending = 0;
            sv->clients[i].queue = NULL;
            sv->clients[i].queue_tail = NULL;
            if (i >= sv->client_count)
                sv->client_count = i + 1;
            return i;
        }
    }
    close(fd);
    return -1;
}

static void remove_client(server_t *sv, int idx)
{
    client_t *c = &sv->clients[idx];
    cmd_t *cmd;

    if (c->player)
        kill_player(sv, c->player);
    while (c->queue) {
        cmd = c->queue;
        c->queue = cmd->next;
        free(cmd->line);
        free(cmd);
    }
    if (c->fd >= 0)
        close(c->fd);
    memset(c, 0, sizeof(*c));
    c->fd = -1;
}

static void auth_player(server_t *sv, client_t *c, const char *team_name)
{
    team_t *team = find_team(sv, team_name);
    char msg[64];

    if (!team || team->slots <= 0) {
        send_fd(c->fd, "ko\n");
        remove_client(sv, c - sv->clients);
        return;
    }
    team->slots--;
    c->type = CT_PLAYER;
    c->team = team;
    snprintf(msg, sizeof(msg), "%d\n", team->slots);
    send_fd(c->fd, msg);
    snprintf(msg, sizeof(msg), "%d %d\n", sv->width, sv->height);
    send_fd(c->fd, msg);
    c->player = spawn_player(sv, team, (int)(c - sv->clients));
}

static void process_line(server_t *sv, client_t *c, char *line)
{
    if (line[0] == '\0')
        return;
    if (c->type == CT_AUTH) {
        if (strcmp(line, "GRAPHIC") == 0) {
            c->type = CT_GUI;
            send_fd(c->fd, "WELCOME\n");
            gui_send_initial_state(sv, c);
            return;
        }
        auth_player(sv, c, line);
        return;
    }
    if (c->type == CT_GUI)
        handle_gui_cmd(sv, c, line);
    else if (c->type == CT_PLAYER)
        handle_player_cmd(sv, c, line);
}

static void read_client(server_t *sv, int idx)
{
    client_t *c = &sv->clients[idx];
    char *nl;
    int n;

    n = recv(c->fd, c->buf + c->buf_len, READ_BUF - c->buf_len - 1, 0);
    if (n <= 0) {
        remove_client(sv, idx);
        return;
    }
    c->buf_len += n;
    c->buf[c->buf_len] = '\0';
    while ((nl = strchr(c->buf, '\n')) != NULL) {
        *nl = '\0';
        process_line(sv, c, c->buf);
        if (c->fd < 0)
            return;
        c->buf_len -= (int)(nl + 1 - c->buf);
        memmove(c->buf, nl + 1, c->buf_len + 1);
    }
}

static int setup_socket(int port)
{
    struct sockaddr_in addr;
    int fd;
    int opt = 1;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
        return -1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0
        || listen(fd, 32) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int64_t next_wakeup(server_t *sv)
{
    int64_t n = now_us();
    int64_t wake = n + tu_to_us(sv, 1);

    for (int i = 0; i < sv->client_count; i++) {
        cmd_t *cmd = sv->clients[i].queue;
        if (cmd && cmd->exec_at < wake)
            wake = cmd->exec_at;
    }
    for (incant_t *inc = sv->incants; inc; inc = inc->next) {
        if (inc->end_at < wake)
            wake = inc->end_at;
    }
    for (int i = 0; i < sv->player_count; i++) {
        player_t *p = sv->players[i];
        if (p && p->alive && p->next_food_tick < wake)
            wake = p->next_food_tick;
    }
    return wake;
}

static void read_admin(server_t *sv)
{
    char buf[256];
    ssize_t n;

    n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
    if (n <= 0)
        return;
    buf[n] = '\0';
    if (buf[n - 1] == '\n')
        buf[n - 1] = '\0';
    handle_admin(sv, buf);
}

void server_run(server_t *sv)
{
    fd_set rfds;
    struct timeval tv;
    int64_t wake;

    while (!sv->game_over) {
        int maxfd = sv->sock_fd;

        FD_ZERO(&rfds);
        FD_SET(sv->sock_fd, &rfds);
        FD_SET(STDIN_FILENO, &rfds);
        if (STDIN_FILENO > maxfd)
            maxfd = STDIN_FILENO;
        for (int i = 0; i < sv->client_count; i++) {
            if (sv->clients[i].fd >= 0) {
                FD_SET(sv->clients[i].fd, &rfds);
                if (sv->clients[i].fd > maxfd)
                    maxfd = sv->clients[i].fd;
            }
        }
        wake = next_wakeup(sv);
        tv.tv_sec = 0;
        tv.tv_usec = (wake - now_us());
        if (tv.tv_usec < 0)
            tv.tv_usec = 0;
        if (tv.tv_usec > 1000000)
            tv.tv_usec = 100000;
        select(maxfd + 1, &rfds, NULL, NULL, &tv);
        if (FD_ISSET(sv->sock_fd, &rfds)) {
            int cfd = accept(sv->sock_fd, NULL, NULL);
            int idx;

            if (cfd >= 0) {
                idx = add_client(sv, cfd);
                if (idx >= 0)
                    send_fd(cfd, "BIENVENUE\n");
            }
        }
        for (int i = 0; i < sv->client_count; i++)
            if (sv->clients[i].fd >= 0 && FD_ISSET(sv->clients[i].fd, &rfds))
                read_client(sv, i);
        if (FD_ISSET(STDIN_FILENO, &rfds))
            read_admin(sv);
        world_tick(sv);
        process_queues(sv);
    }
}

server_t *server_create(args_t *args)
{
    server_t *sv = calloc(1, sizeof(server_t));

    if (!sv)
        return NULL;
    sv->port = args->port;
    sv->width = args->width;
    sv->height = args->height;
    sv->freq = args->freq;
    sv->next_player_id = 1;
    sv->next_egg_id = 1;
    sv->start_us = now_us();
    for (int i = 0; i < MAX_CLIENTS; i++)
        sv->clients[i].fd = -1;
    sv->map = calloc(args->height, sizeof(tile_t *));
    for (int y = 0; y < args->height; y++)
        sv->map[y] = calloc(args->width, sizeof(tile_t));
    team_t *tail = NULL;
    for (int i = 0; i < args->team_count; i++) {
        team_t *t = calloc(1, sizeof(team_t));
        t->name = strdup(args->team_names[i]);
        t->slots = args->clients_per_team;
        t->max_clients = args->clients_per_team;
        if (!sv->teams)
            sv->teams = t;
        else
            tail->next = t;
        tail = t;
    }
    srand((unsigned)now_us());
    spawn_resources(sv, true);
    sv->next_spawn_tick = SPAWN_INTERVAL;
    sv->sock_fd = setup_socket(args->port);
    if (sv->sock_fd < 0) {
        server_destroy(sv);
        return NULL;
    }
    return sv;
}

void server_destroy(server_t *sv)
{
    if (!sv)
        return;
    for (int i = 0; i < sv->client_count; i++)
        remove_client(sv, i);
    if (sv->sock_fd >= 0)
        close(sv->sock_fd);
    for (int i = 0; i < sv->player_count; i++)
        free(sv->players[i]);
    free(sv->players);
    for (team_t *t = sv->teams; t;) {
        team_t *n = t->next;
        free(t->name);
        free(t);
        t = n;
    }
    if (sv->map) {
        for (int y = 0; y < sv->height; y++) {
            for (int x = 0; x < sv->width; x++) {
                egg_t *e = sv->map[y][x].eggs;
                while (e) {
                    egg_t *n = e->next;
                    free(e);
                    e = n;
                }
            }
            free(sv->map[y]);
        }
        free(sv->map);
    }
    while (sv->incants) {
        incant_t *n = sv->incants->next;
        free(sv->incants->participants);
        free(sv->incants);
        sv->incants = n;
    }
    free(sv);
}
