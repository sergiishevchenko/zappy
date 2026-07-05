#include "zappy.h"

void gui_smg(server_t *sv, const char *msg)
{
    char line[512];

    snprintf(line, sizeof(line), "smg %s\n", msg);
    gui_broadcast(sv, line);
}

void handle_admin(server_t *sv, const char *line)
{
    char cmd[64];
    char arg[256];
    char msg[512];

    if (sscanf(line, "%63s %255[^\n]", cmd, arg) < 1)
        return;
    if (strcmp(cmd, "freq") == 0 && arg[0]) {
        int f = atoi(arg);
        if (f > 0) {
            sv->freq = f;
            snprintf(msg, sizeof(msg), "sst %d\n", sv->freq);
            gui_broadcast(sv, msg);
            fprintf(stderr, "freq set to %d\n", f);
        }
    } else if (strcmp(cmd, "status") == 0) {
        fprintf(stderr, "players=%d freq=%d map=%dx%d\n",
            sv->player_count, sv->freq, sv->width, sv->height);
        for (team_t *t = sv->teams; t; t = t->next)
            fprintf(stderr, "  %s slots=%d L8=%d\n", t->name, t->slots,
                count_team_level8(sv, t));
    } else if (strcmp(cmd, "msg") == 0 && arg[0]) {
        gui_smg(sv, arg);
        fprintf(stderr, "broadcast: %s\n", arg);
    } else if (strcmp(cmd, "help") == 0) {
        fprintf(stderr, "admin: freq <n> | status | msg <text> | help\n");
    }
}
