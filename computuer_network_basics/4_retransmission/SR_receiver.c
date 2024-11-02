#include "Common.h"
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>

char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512
#define PKT_INTERVAL 5
#define PKTSIZE 8
#define ACKSIZE 5

SOCKET sock;
int expected_pkt = -1;
int buffer[10] = {0};

void* recv_pck(void* arg) {
        sleep(1);
        char buf[BUFSIZE];
        int len_recv;
        int* ack_send = (int*)arg;
        len_recv = recv(sock, buf, PKTSIZE, MSG_WAITALL);
        buf[PKTSIZE] = '\0';
        // pkt loss 
        if (len_recv <= 0) pthread_exit(NULL);
        
        int received_pkt_num = atoi(buf + 7); 
        if (received_pkt_num == expected_pkt + 1) {
            // 정상 ACK
                expected_pkt++;
                printf("\"%s\" is received\" ACK %d\" is transmitted\n",buf, expected_pkt);
                sprintf(buf, "ACK %d", expected_pkt);
                len_recv = send(sock, buf, ACKSIZE, 0);
        }
        // 제대로 안 왔다. pkt3 -> ack3 and buffering 
        else {
            if(received_pkt_num > expected_pkt +1 && received_pkt_num < received_pkt_num + 2 ){
                buffer[received_pkt_num] = received_pkt_num;
                printf("\"%s\" is received and buffered. \"ACK %d\" is retransmitted.\n", buf, received_pkt_num);
                sprintf(buf, "ACK %d", received_pkt_num);
                len_recv = send(sock, buf, ACKSIZE, 0);
            }
        }
        pthread_exit(NULL);
}


int main(int argc, char *argv[])
{
        int retval;
        int ack_send = 1;       
        pthread_t tid;

        if (argc > 1) SERVERIP = argv[1];
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) err_quit("socket()");

        // connect()
        struct sockaddr_in serveraddr;
        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);

        serveraddr.sin_port = htons(SERVERPORT);
        retval = connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
        if (retval == SOCKET_ERROR) err_quit("connect()");

        // time Interval 5초
        struct timeval timeout;
        timeout.tv_sec = PKT_INTERVAL;
        timeout.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
        
        // 조건에서 5번째 이상으로는 ack을 주지 않아도됨.
        for(int i = 0 ; i< 6 ; i++) {
            ack_send = i;
            pthread_create(&tid, NULL, recv_pck, &ack_send);
            pthread_join(tid, NULL);  
        }
        // 재전송 이후, pkt2 송신과 buffer 패킷 전달
        char buf[BUFSIZE];
        retval =recv(sock,buf,8,0);
        printf("\"%s\" is received. %s, ", buf, buf);
        for(int i =0;i<10; i++) {
            if(buffer[i] != 0 ){ 
                if(i != 5){
                    printf("packet %d, ", buffer[i]);
                    }
                else {
                    printf("and packet 5");
                }
            }
        }
        printf(" are delivered. \"ACK %c\" is transmitted\n",buf[7]);
        close(sock);
        return 0;
}