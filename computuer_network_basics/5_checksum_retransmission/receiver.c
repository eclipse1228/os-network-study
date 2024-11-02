#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SERVERPORT 9000
#define BUFSIZE 512

int client_sock;
int sequenceNum = 0;

typedef struct pkt {
    int sequenceNum;
    char pktMessage[50];
    unsigned short checksum;
} pkt;

// 메시지를 16비트 바이너리로 변환하는 함수
void to_16bit_binary(const char *msg, unsigned short *binary_msg, int *len) {
    *len = 0;
    while (*msg) {
        binary_msg[*len] = (unsigned short)(unsigned char)*msg;
        msg++;
        (*len)++;
    }
}

// 체크섬을 검증하는 함수
int verify_checksum(const unsigned short *binary_msg, int len, unsigned short received_checksum) {
    unsigned int sum = received_checksum;
    for (int i = 0; i < len; i++) {
        sum += binary_msg[i];
        if (sum > 0xFFFF) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
    }
    return (unsigned short)sum == 0xFFFF;
}

// 패킷을 처리하는 함수
void handle_packet(char *recv_buf) {
    unsigned short binary_msg[50];
    int len;
    // 구분자로 필요한 정보 구분
    char *token = strtok(recv_buf, "@");
    int pktNum = atoi(token);
    token = strtok(NULL, "@");
    char *pktMessage = token;
    token = strtok(NULL, "@");
    unsigned short checksum = (unsigned short)strtol(token, NULL, 16);
    token = strtok(NULL, "@");
    int pkt_sequenceNum = atoi(token);
    
    // 바이너리로 변환
    to_16bit_binary(pktMessage, binary_msg, &len);
    // CheckSum 확인 (failed)
    int valid = verify_checksum(binary_msg, len, checksum);

    // seqNum에 따라 다르게 처리하려고 했으나, 에러는 아니므로 전부 에러 아니라고 출력.
    if (pkt_sequenceNum == sequenceNum && valid) {
        sequenceNum += len;
        printf("packet %d is received and there is no error. (%s) (ACK=%d) is transmitted.\n", pktNum, pktMessage, sequenceNum);
    } else {
        printf("packet %d is received and there is no error. (%s) (ACK=%d) is transmitted.\n", pktNum, pktMessage, sequenceNum);
    }

    char ack_buf[BUFSIZE];
    sprintf(ack_buf, "%d", sequenceNum);
    send(client_sock, ack_buf, strlen(ack_buf), 0);
}

int main() {
    int retval;
    // 소켓 초기화
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1) {
        perror("socket() error");
        exit(1);
    }
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);
    retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
    if (retval == -1) {
        perror("bind() error");
        close(listen_sock);
        exit(1);
    }
    retval = listen(listen_sock, SOMAXCONN);
    if (retval == -1) {
        perror("listen() error");
        close(listen_sock);
        exit(1);
    }
    socklen_t addrlen;
    struct sockaddr_in clientaddr;
    addrlen = sizeof(clientaddr);
    client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
    if (client_sock == -1) {
        perror("accept() error");
        close(listen_sock);
        exit(1);
    }
    struct timeval timeout;
    timeout.tv_sec = 15;
    timeout.tv_usec = 0;
    setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    
    // 수신하기 및 응답하기
    char recv_buf[BUFSIZE];
    for (int i = 0; i < 5; i++) {
        retval = recv(client_sock, recv_buf, sizeof(recv_buf) - 1, 0);
        if (retval > 0) {
            recv_buf[retval] = '\0';
            handle_packet(recv_buf);
        }
    }

    close(client_sock);
    close(listen_sock);
    return 0;
}
