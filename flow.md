# 구현 방향
## 목표와 제약 확정

ft_ping <host> 실행시
- ICMP Echo Request 전송
- Echo Reply 수신
- RTT 출력
- Ctrl+C 시 통계 출력

## "호스트를 IP로 바꾸기" (DNS/해석)

### 왜 해야 하나

ping google.com 같은 입력은 도메인이다. \
Raw Socket으로 패킷을 보내려면 **목적지 IP**가 필요함.

### 무엇을 얻어야 하나
- struct sockaddr_in dst (IPv4 주소 + 포트(0))
- 출력용 문자욜 IP ("152.250...")

### 어떤 함수/구조체를 쓰나

`getaddrinfo()`
- 도메인 -> 주소 목록을 얻는 표준 API
- AF_INET으로 제한하면 IPv4만 받음
- ping 구현에서 거의 정석

`sockaddr_in`
- sendto()가 요구하는 목적지 주소 구조체

`inet_ntop()`
- sockaddr_in 안의 바이너리 IP를 문자열로 변환(출력용)

## "Raw Socket 만들기" (ICMP를 다루는 통로)

### Why?
ICMP는 TCP/UDP가 아니다. 포트가 없다 \
그래서 SOCK_STREAM/SOCK_DGRAM으로는 ping을 구현할 수 없다.

그러면? 커널에게 ICMP 패킷을 직접 주고받겠다 고 선언하는 통로가 필요하다. \
그게 Raw Socket이다.

### 어떤 함수가 필요한가

`socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)`
- IPv4 + Raw + ICMP 프로토콜
- 이 소켓으로 `sendto/recvfrom` 하면 ICMP를 직접 주고 받을 수 있다.

### 왜 root가 필요한가?
Raw socket은 보안상 위험하다(임의 패킷 생성 가능).\
그래서 기본적으로 root만 가능.

## ICMP Echo Request 패킷 만들기 + 전송
### 왜 "패킷을 직접 만들어야" 하나
Raw socket을 쓰면 커널이 TCP/UDP 헤더를 만들어 주지 않는다. \
ICMP ping에서는 우리가 최사한 아래를 직접 채워야 한다.

- ICMP 헤더(타입/코드/ID/SEQ)
- payload(데이터)
- checksum

이 셋이 맞아야 상대가 응답한다.

### 무엇을 만들어야 하나 (구성요소)

ICMP Echo Request는 구조적으로 다음이다.

```bash
[ ICMP header ][ payload ]
```


Linux에서는 `struct icmphdr`를 쓴다
- type: Echo Request면 8
- code: 0
- id: 프로세스 식별자 (보통 pid)
- sequence: 1,2,3... 증가
- checksum: 전체(ICMP header + payload)에 대한 체크섬

payload는 그냥 아무 바이트나 넣어도 된다
다만 다음 단계(RTT)에서 편하려면 **payload 앞부분에 "보낸 시간"** 을 넣는다.

### 왜 checksum이 필요한가

ICMP는 무결성 검사를 위해 checksum을 사용한다.\
checksum이 틀리면 상대는 조용히 버린다. (에러 없음)

그래서 ping 구현에서 가장 흔한 실패 원인이 checksum이다.

### 어떤 함수/구조체가 필요한가

`struct icmphdr (<netinet/ip_icmp.h>)`

ICMP 헤더를 표준 형태로 다룬다.

`gettimeofday() (<sys/time.h>)`

payload에 보낸 시간을 넣기 위해 사용한다.

`sendto() (<sys/socket.h>)`

구성한 패킷을 목적지로 전송한다.

`icmp_checksum() (직접 구현)`

ICMP checksum은 OS가 안해줌으로 직접 구현해야 함.


