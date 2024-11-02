#include "Common.h"
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define SERVERPORT 9000
#define BUFSIZE    512
#define PKT_INTERVAL 5

int window_Start = -1;          // slide window 시작 Index
int window_End = 3;             // slide winodw 끝 Index
int pktCount[7];                // Ack 저장 배열 0~6 pkt
SOCKET client_sock;             // 통신 소켓
int timeout = 1;                // timeout은 1번만 출력해야하는 조건.
int record = -1;                // record 변수

/* TimeOut 이후 recv_Ack 함수*/
void* after_timeOut() {
    sleep(1); 
    char buf[BUFSIZE];
    int len_recv;

    while (1) {
        len_recv = recv(client_sock, buf, 5, 0);
        if (len_recv <= 0) {
            break;
        }
        buf[len_recv] = '\0';  
        int ackNum = atoi(buf + 4);  // ACK 모양 =="ACK X"

        if (ackNum == window_Start + 1) {
            // 정상 ACK 수신 
            window_Start++;
            window_End++;

            sprintf(buf, "Packet %d", window_End);
            send(client_sock, buf, 8, 0);
            sleep(1); 
        } 
    }
    pthread_exit(NULL);
}

void* recv_Ack() {
    sleep(1);
    char buf[BUFSIZE];
    int len_recv;

    while (1) {
        len_recv = recv(client_sock, buf, 5, 0);
        if (len_recv <= 0) {
            // timeOut
            if(timeout){
                timeout = 0;
                printf("\"Packet %d\" is timeout.\n", window_Start + 1);
                sprintf(buf, "Packet %d", window_Start);
                send(client_sock,buf,8,0);
                break;
            }
            else {
                break;
            }
        }
        buf[len_recv] = '\0';  
        int ackNum = atoi(buf + 4);  //"ACK X"
        // 비정상 ACK 
        if (ackNum > window_Start+ 1 && ackNum < window_End && ackNum < 4) { 
            // 비정상 pkt 저장 recorded (ack3) 조건상 pkt 4 이상은 record x
            record = ackNum;
            printf("\"%s\" received and recorded.\n", buf);
            break;
        }

        else if (ackNum == window_Start + 1) {
            // 정상 ACK
            window_Start++;
            window_End++;
            printf("\"%s\" received. \"Packet %d\" is transmitted.\n", buf, window_End);
            sprintf(buf, "Packet %d", window_End);
            send(client_sock, buf, 8, 0);
            sleep(1);
        } 
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
        int retval;
        pthread_t tid[4];

        // 소켓 생성
        SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sock == INVALID_SOCKET) err_quit("socket()");

        struct sockaddr_in serveraddr;
        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

        serveraddr.sin_port = htons(SERVERPORT);
        retval = bind(listen_sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
        if (retval == SOCKET_ERROR) err_quit("bind()");

        retval = listen(listen_sock, SOMAXCONN);
        if (retval == SOCKET_ERROR) err_quit("listen()");

        socklen_t addrlen;
        struct sockaddr_in clientaddr;
        char buf[BUFSIZE + 1];

        // accept()
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (struct sockaddr *)&clientaddr, &addrlen);
        
        char addr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
        // TimeInterval 5
        struct timeval timeout;
        timeout.tv_sec = PKT_INTERVAL;
        timeout.tv_usec= 0;
        setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
        
        // 패킷 전송 pkt 2손실
        for(int i = 0; i < 4; i++) {
                sprintf(buf, "packet %d", i);
                printf("\"%s\" is transmitted.\n", buf);
                if(i != 2) send(client_sock, buf, 8, 0);
                pthread_create(&tid[i], NULL, recv_Ack, NULL);
        }
        
        for(int i = 0; i < 4; i++) {
                pthread_join(tid[i], NULL);
        }

        // timeOut 이후 pkt2 송신
        sprintf(buf, "packet 2");
        printf("\"%s\" is transmitted.\n", buf);
        send(client_sock,buf,8,0);
        sleep(1);

        close(client_sock);
        close(listen_sock);
        return 0;
}
