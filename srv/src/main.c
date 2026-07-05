#include "zappy.h"

int main(int ac, char **av)
{
    args_t args;
    server_t *sv;

    if (ac < 2 || (ac >= 2 && strcmp(av[1], "-h") == 0)) {
        print_usage(av[0]);
        return ac < 2 ? 84 : 0;
    }
    if (parse_args(ac, av, &args) < 0) {
        print_usage(av[0]);
        return 84;
    }
    sv = server_create(&args);
    if (!sv) {
        free_args(&args);
        return 84;
    }
    server_run(sv);
    server_destroy(sv);
    free_args(&args);
    return 0;
}
