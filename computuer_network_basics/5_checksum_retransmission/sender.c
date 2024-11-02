#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define pktCount 5
#define SERVERPORT 9000
#define BUFSIZE 512

char *SERVERIP = "127.0.0.1";
int sock;
char ack_buf[BUFSIZE];
int ack_count[100] = {0};
int retrans = 0;
int retrans_Ack = -1;
char ack_temp_buf[BUFSIZE * 10];    // ACK 메세지를 보관하는 버퍼(연속으로 출력되는 ACK)

typedef struct pkt {
    int sequenceNum;
    char pktMessage[50];
    unsigned short checksum;
} pkt;

int sequenceNum = 0;
char pktMessage[][50] = {
    "I am a boy.",
    "You are a girl.",
    "There are many animals in the zoo.",
    "철수와 영희는 서로 좋아합니다!",
    "나는 점심을 맛있게 먹었습니다."
};

// 1의 보수를 계산하는 함수
unsigned short oneComplement(unsigned short sum) {
    return ~sum;
}

// 체크섬을 계산하는 함수
unsigned short calcul_checkSum(const unsigned short *binary_msg, int len) {
    unsigned int sum = 0;
    for (int i = 0; i < len; i++) {
        sum += binary_msg[i];
        if (sum > 0xFFFF) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
    }
    return oneComplement((unsigned short)sum);
}

// 메시지를 16비트 바이너리로 변환하는 함수
void to_16bit_binary(const char *msg, unsigned short *binary_msg, int *len) {
    *len = 0;
    while (*msg) {
        binary_msg[*len] = (unsigned short)(unsigned char)*msg;
        msg++;
        (*len)++;
    }
}

// 중복 유무 확인 
int check_dupl() {
    for (int i = 0; i < 100; i++) {
        if (ack_count[i] >= 4) {
            retrans_Ack = i;
            retrans = 1;
        }
    }
    return retrans;
}
// ACK 수신 함수 (Thread)
void* ack_recv(void* arg) {
    char local_ack_buf[BUFSIZE];
    while (1) {
        int retval = recv(sock, local_ack_buf, sizeof(local_ack_buf) - 1, 0);
        if (retval <= 0) continue;
        local_ack_buf[retval] = '\0';
        strcat(ack_temp_buf, "(ACK = ");
        strcat(ack_temp_buf, local_ack_buf);
        strcat(ack_temp_buf, ") is received.\n");
        int ack_num = atoi(local_ack_buf);
        ack_count[ack_num] += 1;
        check_dupl();
        if (ack_num == 11 && ack_count[ack_num] == 4) {
            break;
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc > 1) SERVERIP = argv[1];
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket() error");
        exit(1);
    }

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(SERVERPORT);
    if (connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1) {
        perror("connect() error");
        close(sock);
        exit(1);
    }

    struct timeval timeout; // 소켓 타임아웃 15
    timeout.tv_sec = 15;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    // 패킷 메세지 5개 , 바이너리는 50까지
    pkt packets[5];
    unsigned short binary_msg[50];
    int len;

    /* 패킷 초기화 */
    for (int i = 0; i < 5; i++) {
        strcpy(packets[i].pktMessage, pktMessage[i]);           // 패킷의 메세지
        to_16bit_binary(packets[i].pktMessage, binary_msg, &len); // 16비트 바이너리
        packets[i].checksum = calcul_checkSum(binary_msg, len); // 체크섬 계산
        packets[i].sequenceNum = sequenceNum;                   // 순서번호
        sequenceNum += len;                                     // 순서번호 업데이트
    }

    // ACK 수신 쓰레드 
    pthread_t tid;
    pthread_create(&tid, NULL, ack_recv, NULL);

    // 패킷 전송 ('@'로 보내야할 정보를 구분했다.)
    for (int i = 0; i < 5; i++) {
        if (i != 1) {
            char buf[BUFSIZE];
            sprintf(buf, "%d@%s@%04X@%d", i, packets[i].pktMessage, packets[i].checksum, packets[i].sequenceNum);
            send(sock, buf, strlen(buf), 0);
            printf("packet %d is transmitted. (%s)\n", i, packets[i].pktMessage);
        } else {
            printf("packet %d is transmitted. (%s)\n", i, packets[i].pktMessage);
        }
        usleep(500000); // 0.5초 대기하여 ACK 수신을 기다림
    }

    pthread_join(tid, NULL);
    printf("%s", ack_temp_buf);

    // 패킷 재전송이 필요하다.
    if (retrans) {
        for (int i = 0; i < 5; i++) {
            if (packets[i].sequenceNum == retrans_Ack) {
                char buf[BUFSIZE];
                sprintf(buf, "%d@%s@%04X@%d", i, packets[i].pktMessage, packets[i].checksum, packets[i].sequenceNum);
                send(sock, buf, strlen(buf), 0);
                printf("packet %d is retransmitted. (%s)\n", i, packets[i].pktMessage);
                break;
            }
        }
    }

    close(sock);
    return 0;
}
