#include "Common.h"
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
/* 각 쓰레드는 패킷 하나를 관리한다. 비효율적인 방법일 수 있으나, 동기화를 위해 가장 확실한 방법
비동기, Socket Select() 대기 보다 간단한 제어 방법이다.
 */
char *SERVERIP = (char *)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE    512
#define PKT_INTERVAL 5
#define PKTSIZE 8
#define ACKSIZE 5

SOCKET sock;                            
int expected_pkt = -1;                  // 예상되는 패킷 번호 

/* receive 함수 (쓰레드가 사용)*/
void* recv_pck(void* arg) {
        sleep(1);
        char buf[BUFSIZE];
        int len_recv;
        int* ack_send = (int*)arg;
        len_recv = recv(sock, buf, PKTSIZE, MSG_WAITALL);
        buf[PKTSIZE] = '\0';

        if (len_recv <= 0) pthread_exit(NULL);
        
        int received_pkt_num = atoi(buf + 7);               // 패킷 번호만 정수로 추출
        if (received_pkt_num == expected_pkt + 1) {
                // 예상한 패킷일 때
                expected_pkt++;
                if(*ack_send >= 5) {
                        // 재전송 (5번째 이상 전송은 재전송) 
                        printf("\"%s\" is receive and delivered. \"ACK %d\" is transmitted\n",buf, expected_pkt);
                        sprintf(buf, "ACK %d", expected_pkt);
                        len_recv = send(sock, buf, ACKSIZE, 0);
                }
                else {
                        // 정상 전송
                        printf("\"%s\" is received\"ACK %d\" is transmitted\n",buf, expected_pkt);
                        sprintf(buf, "ACK %d", expected_pkt);
                        len_recv = send(sock, buf, ACKSIZE, 0);
                }
        }
        else {
                // 예상 패킷이 아닌 경우 Drop && 이전 번호 ACK 재전송
                printf("\"%s\" is received and dropped. \"ACK %d\" is retransmitted.\n", buf, expected_pkt);
                buf[0]='\0';
                sprintf(buf, "ACK %d", expected_pkt);
                len_recv = send(sock, buf, ACKSIZE, 0);
        }
        pthread_exit(NULL);
}


int main(int argc, char *argv[])
{
        int retval;
        int ack_send = 1;               // 조건에서 타임아웃으로 넘어오는 패킷에 대한 ack 언급 없음으로 보내지않음
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
        for(int i = 0 ; i< 9 ; i++) {
            //if(i == 5) ack_send = 0;
            ack_send = i;
            pthread_create(&tid, NULL, recv_pck, &ack_send);
            pthread_join(tid, NULL);        
        }

        close(sock);
        return 0;
}