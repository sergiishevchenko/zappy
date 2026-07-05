#include "gfx.hpp"
#include <cstdio>
#include <cstring>
#include <cstdlib>

int main(int ac, char **av)
{
    const char *host = "localhost";
    int port = 0;

    if (ac < 2) {
        fprintf(stderr, "USAGE: %s -p <port> -h <machine>\n", av[0]);
        return 84;
    }
    for (int i = 1; i < ac; i++) {
        if (strcmp(av[i], "-p") == 0 && i + 1 < ac)
            port = atoi(av[++i]);
        else if (strcmp(av[i], "-h") == 0 && i + 1 < ac)
            host = av[++i];
        else if (strcmp(av[i], "-help") == 0) {
            fprintf(stderr, "USAGE: %s -p <port> -h <machine>\n", av[0]);
            return 0;
        }
    }
    if (port <= 0) {
        fprintf(stderr, "USAGE: %s -p <port> -h <machine>\n", av[0]);
        return 84;
    }
    return run_gfx(host, port);
}
