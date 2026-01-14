#include "include/ft_ping.h"

int main(int ac, char **av) {
    if (ac == 1)
        return (no_ac());

    char test[INET_ADDRSTRLEN];
    size_t ip_str_len = INET_ADDRSTRLEN;
    struct sockaddr_in out;
    if (ac == 2)
    {
        if (strcmp("-?", av[1]) == 0)
            return (print_help());
        else if (strcmp("-V", av[1]) == 0)
            return (print_version());
        else
        {
            if (resolve_ipv4("google.com", &out, test, ip_str_len) != 0)
            {
                fprintf(stderr, "Failed to resolve host\n");
                return 1;
            }
        }
    }

    fprintf(stdout, "%s\n", test);
    return 0;
}