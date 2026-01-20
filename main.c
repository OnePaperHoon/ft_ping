#include "include/ft_ping.h"
#include <netinet/in.h>

volatile sig_atomic_t g_stop = 0;

static void on_sigint(int signo)
{
    (void)signo;
    g_stop = 1;
}

int main(int ac, char **av)
{
    if (ac == 1)
        return no_ac();

    if (ac != 2)
    {
        fprintf(stderr, "ft_ping: too many arguments\n");
        return 2;
    }

    if (strcmp("-?", av[1]) == 0)
        return print_help();
    if (strcmp("-V", av[1]) == 0)
        return print_version();

    char ip_str[INET_ADDRSTRLEN];
    struct sockaddr_in out;

    if (resolve_ipv4(av[1], &out, ip_str, sizeof(ip_str)) != 0)
    {
        fprintf(stderr, "Failed to resolve host\n");
        return 1;
    }

    int sock = open_icmp_socket();
    if (sock < 0)
    {
        fprintf(stderr, "Failed to open ICMP socket\n");
        return 1;
    }

    printf("PING %s (%s): %d data bytes\n", av[1], ip_str, PAYLOAD_SIZE);

    uint16_t ident = (uint16_t)(getpid() & 0xFFFF);
    uint16_t seq = 0;
    unsigned long transmitted = 0;
    unsigned long received = 0;

    // 통계 변수
    double rtt_min = 1e18;
    double rtt_max = 0.0;
    double rtt_sum = 0.0;
    double rtt_sum_sq = 0.0;

    double rtt = 0.0;
    uint8_t ttl = 0;
    int icmp_bytes = 0;
    struct sockaddr_in src;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigaction(SIGINT, &sa, NULL);

    while (!g_stop)
    {
        long long loop_start = now_ms();
        

        // 1) 송신
        if (send_echo_request(sock, &out, ident, seq) != 0)
        {
            perror("sendto");
        }
        else
        {
            transmitted++;
            // 2) 최대 1초 수신 대기
            int r = wait_readable(sock, 1000);
            if (r < 0)
            {
                if (errno != EINTR)
                    perror("select");
            }
            else if (r == 1)
            {
                // 3) 내 reply를 만날 때까지(또는 더 읽을 게 없을 때까지) 몇 개 소진
                while (1)
                {
                    if (receive_echo_reply(sock, ident, seq, &src, &rtt, &ttl, &icmp_bytes) == 0)
                    {
                        char src_ip[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &src.sin_addr, src_ip, sizeof(src_ip));

                        printf("%d bytes from %s: icmp_seq=%u ttl=%u time=%.3f ms\n",
                               icmp_bytes, src_ip, (unsigned)seq, (unsigned)ttl, rtt);
                        received++;
                        if (rtt < rtt_min)
                            rtt_min = rtt;
                        if (rtt > rtt_max)
                            rtt_max = rtt;
                        rtt_sum += rtt;
                        rtt_sum_sq += rtt * rtt;
                        break;
                    }
                    
                    // 더 읽을 게 없으면 빠져나오기(블로킹 방지)
                    int rr = wait_readable(sock, 0);
                    if (rr <= 0)
                        break;
                }
            }
            // r == 0 이면 timeout: inetutils 스타일이면 출력 없이 넘어감
        }

        // 4) 총 루프 시간이 1초가 되도록 남은 시간 sleep
        long long elapsed = now_ms() - loop_start;
        long long remain = 1000 - elapsed;
        if (remain > 0)
            sleep_ms(remain);
        seq++;
    }
    printf("--- %s ping statistics ---\n", av[1]);

    unsigned long loss = 0;
    if (transmitted > 0)
        loss = (unsigned long)((100 * (transmitted - received)) / transmitted);

    printf("%lu packets transmitted, %lu packets received, %lu%% packet loss\n",
       transmitted, received, loss);

        if (received > 0) {
        double avg = rtt_sum / (double)received;
        double var = (rtt_sum_sq / (double)received) - (avg * avg);
        
        if (var < 0.0) var = 0.0;
        double stddev = sqrt(var);

        printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
               rtt_min, avg, rtt_max, stddev);
    }
    close(sock);
    return 0;
}