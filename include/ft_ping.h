#ifndef FT_PING_H
#define FT_PING_H

#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <netinet/ip_icmp.h>
#include <sys/time.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <math.h>

#define PAYLOAD_SIZE 56


int print_help(void);
int print_version(void);
int no_ac(void);
int resolve_ipv4(const char *host, struct sockaddr_in *out, char *ip_str, size_t ip_str_sz);
int open_icmp_socket(void);
uint16_t checksum(const void *data, size_t len);
int send_echo_request(int sock, const struct sockaddr_in *dest_addr, uint16_t ident, uint16_t seq);
int receive_echo_reply( int sock, uint16_t ident, uint16_t seq, struct sockaddr_in *src, double *rtt_ms, uint8_t *ttl_out, int *icmp_bytes_out);
int wait_readable(int sock, int timeout_ms);
long long now_ms(void);
void sleep_ms(long long ms);

#endif