#include "include/ft_ping.h"
#include <netinet/in.h>

// 전역 변수: 프로그램 종료 플래그
volatile sig_atomic_t g_stop = 0;

// SIGINT 핸들러
static void on_sigint(int signo)
{
    (void)signo;
    g_stop = 1;
}

int main(int ac, char **av)
{
    // 인자 개수 검사
    if (ac == 1)
        return no_ac();

    // 너무 많은 인자 검사
    if (ac != 2)
    {
        fprintf(stderr, "ft_ping: too many arguments\n");
        return (2);
    }

    // 도움말 및 버전 정보 출력
    if (strcmp("-?", av[1]) == 0)
        return print_help();
    // 버전 정보 출력
    if (strcmp("-V", av[1]) == 0)
        return print_version();

    // 대상 호스트 해석
    char ip_str[INET_ADDRSTRLEN];

    // 대상 주소 구조체
    struct sockaddr_in out;

    // 도메인 네임을 IPv4 주소로 변환
    if (resolve_ipv4(av[1], &out, ip_str, sizeof(ip_str)) != 0)
    {
        fprintf(stderr, "Failed to resolve host\n");
        return 1;
    }

    // ICMP 소켓 열기
    int sock = open_icmp_socket();
    
    // 소켓 열기 실패 검사
    if (sock < 0)
    {
        fprintf(stderr, "Failed to open ICMP socket\n");
        return 1;
    }

    // 시작 메시지 출력
    printf("PING %s (%s): %d data bytes\n", av[1], ip_str, PAYLOAD_SIZE);

    // 식별자 및 시퀀스 번호 초기화
    uint16_t ident = (uint16_t)(getpid() & 0xFFFF);
    uint16_t seq = 0;
    // 송수신 통계 변수
    unsigned long transmitted = 0;
    unsigned long received = 0;

    // RTT 통계 변수
    double rtt_min = 1e18;
    double rtt_max = 0.0;
    double rtt_sum = 0.0;
    double rtt_sum_sq = 0.0;

    // 수신된 패킷의 RTT, TTL, ICMP 바이트 수 변수
    double rtt = 0.0;
    uint8_t ttl = 0;
    int icmp_bytes = 0;
    struct sockaddr_in src;

    // SIGINT 핸들러 설정
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_sigint;
    sigaction(SIGINT, &sa, NULL);
    
    // 루프 시작 시간 변수
    long long loop_start = 0;
    // 메인 루프
    while (!g_stop)
    {
        // 루프 시작 시간 기록
        loop_start = now_ms();

        // 1) Echo Request 전송
        if (send_echo_request(sock, &out, ident, seq) != 0)
            perror("sendto");
        else
        {
            // 전송된 패킷 수 증가
            transmitted++;
            // 2) 최대 1초 수신 대기
            int r = wait_readable(sock, 1000);
            // 오류 검사
            if (r < 0)
            {
                if (errno != EINTR)
                    perror("select");
            }
            // 수신 가능할 때
            else if (r == 1)
            {
                // 3) 수신된 Echo Reply 처리
                while (1)
                {
                    // Echo Reply 수신 시도
                    if (receive_echo_reply(sock, ident, seq, &src, &rtt, &ttl, &icmp_bytes) == 0)
                    {
                        // 수신된 패킷 정보 출력
                        char src_ip[INET_ADDRSTRLEN];
                        // 출발지 IP 주소 문자열 변환
                        inet_ntop(AF_INET, &src.sin_addr, src_ip, sizeof(src_ip));

                        // 수신된 패킷 정보 출력
                        printf("%d bytes from %s: icmp_seq=%u ttl=%u time=%.3f ms\n",
                               icmp_bytes, src_ip, (unsigned)seq, (unsigned)ttl, rtt);
                        
                        // 수신된 패킷 수 증가
                        received++;

                        // RTT 통계 갱신
                        if (rtt < rtt_min)
                            rtt_min = rtt;
                        if (rtt > rtt_max)
                            rtt_max = rtt;
                        // RTT 합계 및 제곱합 갱신
                        rtt_sum += rtt;
                        rtt_sum_sq += rtt * rtt;
                        // 하나의 패킷만 처리하므로 루프 종료
                        break;
                    }
                    
                    // 더 읽을 게 없으면 빠져나오기
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

    // 통계 출력
    unsigned long loss = 0;

    // 패킷 손실률 계산 
    if (transmitted > 0)
        loss = (unsigned long)((100 * (transmitted - received)) / transmitted);

    printf("%lu packets transmitted, %lu packets received, %lu%% packet loss\n",
       transmitted, received, loss);

        // RTT 통계 출력
        if (received > 0) {
        double avg = rtt_sum / (double)received;
        double var = (rtt_sum_sq / (double)received) - (avg * avg);
        
        // 분산이 음수가 될 수 없도록 보정
        if (var < 0.0) var = 0.0;
        double stddev = sqrt(var);

        printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n",
               rtt_min, avg, rtt_max, stddev);
    }
    close(sock);
    return 0;
}