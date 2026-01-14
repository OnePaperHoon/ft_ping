#include "../include/ft_ping.h"

int print_version(void)
{
    fprintf(stdout, "ping (GNU inetutils) 2.6\n");
    fprintf(stdout, "Copyright (C) 2025 Free Software Foundation, Inc.\n");
    fprintf(stdout, "License GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\n");
    fprintf(stdout, "This is free software: you are free to change and redistribute it.\n");
    fprintf(stdout, "There is NO WARRANTY, to the extent permitted by law.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "Written by Sergey Poznyakoff.\n");

    return 0;
}

int print_help(void)
{
    fprintf(stdout, "Usage: ping [OPTION...] HOST ...\n");
    fprintf(stdout, "Send ICMP ECHO_REQUEST packets to network hosts.\n");
    
    fprintf(stdout, " Options valid for all request types:\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  -n, --numeric              do not resolve host addresses\n");
    fprintf(stdout, "  -r, --ignore-routing       send directly to a host on an attached network\n");
    fprintf(stdout, "      --ttl=N                specify N as time-to-live\n");
    fprintf(stdout, "  -T, --tos=NUM              set type of service (TOS) to NUM\n");
    fprintf(stdout, "  -w, --timeout=N            stop after N seconds\n");
    fprintf(stdout, "  -W, --linger=N             number of seconds to wait for response\n");
    fprintf(stdout, "\n");
    fprintf(stdout, " Options valid for --echo requests:\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  -f, --flood                flood ping (root only)\n");
    fprintf(stdout, "  -l, --preload=NUMBER       send NUMBER packets as fast as possible before\n");
    fprintf(stdout, "  -p, --pattern=PATTERN      fill ICMP packet with given pattern (hex)\n");
    fprintf(stdout, "  -s, --size=NUMBER          send NUMBER data octets\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "  -?, --help                 give this help list\n\n");
    
    fprintf(stdout, "Mandatory or optional arguments to long options are also mandatory or optional\n");
    fprintf(stdout, "for any corresponding short options.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "Options marked with (root only) are available only to superuser.\n");
    fprintf(stdout, "\n");
    fprintf(stdout, "Report bugs to <bug-inetutils@gnu.org>.\n");

    return (0);
}

int no_ac(void)
{
    fprintf(stdout, "ping: missing host operand\nTry 'ping --help' or 'ping --usage' for more information.\n");
    return (64);
}

int resolve_ipv4(const char *host, struct sockaddr_in *out, char *ip_str, size_t ip_str_sz)
{
    struct addrinfo hints;
    struct addrinfo *res = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_RAW; // 의미상 raw 사용(UDP여도 되지만 RAW로 통일)

    int rc = getaddrinfo(host, NULL, &hints, &res); // 도메인 -> IP 주소로 받을수 있음
    if (rc != 0 || res == NULL) {
        fprintf(stderr, "getaddrinfo failed for host %s: %s\n", host, gai_strerror(rc));
        return -1;
    }

    memcpy(out, res->ai_addr, sizeof(struct sockaddr_in));
    inet_ntop(AF_INET, &out->sin_addr, ip_str, (socklen_t)ip_str_sz); 

    freeaddrinfo(res);
    return 0;
}

