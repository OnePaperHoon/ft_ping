#include "include/ft_ping.h"

int main(int ac, char **av) {
    if (ac == 1)
        return (no_ac());

    if (ac == 2)
    {
        if (strcmp("-?", av[1]) == 0)
            return (print_help());
        else if (strcmp("-V", av[1]) == 0)
            return (print_version());
    }

    return 0;
}