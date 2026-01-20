#include "../include/ft_ping.h"
#include <netinet/ip_icmp.h>
#include <stdint.h>

/* 
 * Function: open_icmp_socket
 * --------------------------
 * Raw Socket을 생성하여 ICMP 패킷을 송수신 할 수 있도록 함
 *
 * returns: 성공 시 소켓 디스크립터, 실패 시 -1
 *          	
 */
int open_icmp_socket(void)
{
	int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (sock < 0)
	{
		perror("socket");
		return -1;
	}
	return sock;
}

/*
 * Function: checksum
 * ------------------
 * 데이터의 체크섬을 계산
 *
 * data: 체크섬을 계산할 데이터
 * len: 데이터의 길이
 *
 * returns: 계산된 체크섬 값
 */
uint16_t checksum(const void *data, size_t len)
{
    const uint16_t *ptr = data;
    uint32_t sum = 0;

    while (len > 1)
    {
        sum += *ptr++;
        len -= 2;
    }
    if (len == 1)
    {
        sum += *(const uint8_t *)ptr;
    }

    // 16 비트로 접기
    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}
/*
    계산 과정
    1. 데이터 블록을 16비트 단위로 나누어 모두 더함 (sum += *ptr++;)
    2. 만약 데이터 길이가 홀수라면 마지막 바이트를 16
*/

/* 
 * Function: send_echo_request
 * ---------------------------
 * ICMP Echo Request 패킷을 생성하여 지정된 소켓을 통해 전송
 *
 * sock: ICMP 패킷을 전송할 소켓 디스크립터
 * dest_addr: 목적지 주소 (sockaddr_in 구조체 포인터)
 * ident: 식별자 (Identifier)
 * seq: 시퀀스 번호 (Sequence Number)
 *
 * returns: 성공 시 0, 실패 시 -1
 */
int send_echo_request(int sock, const struct sockaddr_in *dest_addr, uint16_t ident, uint16_t seq)
{
	// 1. 패킷 생성
	uint8_t packet[sizeof(struct icmphdr) + PAYLOAD_SIZE];
	// 2. 패킷 초기화
	memset(packet, 0, sizeof(packet));

	struct icmphdr *ic = (struct icmphdr *)packet;
	ic->type = ICMP_ECHO; // 8
	ic->code = 0;
	ic->un.echo.id = htons(ident);
	ic->un.echo.sequence = htons(seq);

	// payload 맨 앞에 "보낸 시간"을 기록 (다음 단계 RTT 계산용)
	struct timeval tv;
	gettimeofday(&tv, NULL);
	memcpy(packet + sizeof(struct icmphdr), &tv, sizeof(tv));

	// checksum은 계산 전 0 으로 초기화
	ic->checksum = 0;
	ic->checksum = checksum(packet, sizeof(packet));

	ssize_t n = sendto(sock, packet, sizeof(packet), 0,
						(const struct sockaddr *)dest_addr, sizeof(*dest_addr));

	if (n < 0)
	{
		fprintf(stderr, "sendto failed: %s\n", strerror(errno));
		return (-1);
	}

	return (0);
}

int receive_echo_reply(
	int sock,
	uint16_t ident,
	uint16_t seq,
	struct sockaddr_in *src,
	double *rtt_ms,
	uint8_t *ttl_out,
	int *icmp_bytes_out
)
{
	uint8_t buf[1500];
	socklen_t srclen = sizeof(*src);

	ssize_t n = recvfrom(sock, buf, sizeof(buf), 0, (struct sockaddr *)src, &srclen);

	if (n < 0)
		return (-1);

	if ((size_t)n < sizeof(struct iphdr))
		return (-1);
	
	struct iphdr *ip = (struct iphdr *)buf;
	int iphdr_len = ip->ihl * 4;

	if (iphdr_len < 20)
		return (-1);

	if ((size_t)n < (size_t)iphdr_len + sizeof(struct icmphdr))
		return (-1);

	struct icmphdr *ic = (struct icmphdr *)(buf + iphdr_len);

	// 1) Echo Reply 판별
	if (ic->type != ICMP_ECHOREPLY)
		return (-1);

	// 2) id/seq가 내 것 인지 확인
	if ((ntohs(ic->un.echo.id) != ident) || (ntohs(ic->un.echo.sequence) != seq))
		return (-1);
	
	// 3) TTL (IP Header 에서 추출)
	if (ttl_out)
		*ttl_out = (uint8_t)ip->ttl;

	// 4) ICMP bytes (IP Header 제외)
	if (icmp_bytes_out)
		*icmp_bytes_out = (int)(n - iphdr_len);

	// 5) RTT: payload에 넣어둔 timeval 사용
	if ((size_t)n < (size_t)iphdr_len + sizeof(struct icmphdr) + sizeof(struct timeval))
		return (-1);

	struct timeval tv_sent, tv_recv;

	memcpy(&tv_sent, buf + iphdr_len + sizeof(struct icmphdr), sizeof(tv_sent));
	gettimeofday(&tv_recv, NULL);

	*rtt_ms = (tv_recv.tv_sec - tv_sent.tv_sec) * 1000.0 + (tv_recv.tv_usec - tv_sent.tv_usec) / 1000.0;

	return (0);
}