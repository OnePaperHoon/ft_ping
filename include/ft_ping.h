#ifndef FT_PING_H
#define FT_PING_H

#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

int print_help(void);
int print_version(void);
int no_ac(void);
int resolve_ipv4(const char *host, struct sockaddr_in *out, char *ip_str, size_t ip_str_sz);

#endif