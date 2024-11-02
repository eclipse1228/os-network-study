#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define SERVERPORT 9000 // 서버 포트
#define BUFSIZE 1024

int clientSock; // 전역 변수로 선언
struct sockaddr_in clientAddr;
socklen_t clientAddrLen = sizeof(clientAddr);

/*
# 문제
1. 채팅은 순서가 없다. (서버-> 클라 , 클라->서버 둘 다 가능)
2. 스레드생성 함수 pthread_create()에 4번째 파라미터로 3번째에 정한 함수의
파라미터가 들어가는데, 이건 한 개 들어갈 수 있어서, 최대한 넣지 않기 위해 전역으로 소켓 선언.
----??> 그런데 쓰레드 두 개가 하나의 리소스를 공유하면, 경쟁상태가 되서 버그가 날 수도 있다.(메모리 관련) ->
----OK> 그러나, send()와 recv()는 TCP 상에서 괜찮다.(각각 함수 파라미터에 buf를 넣는데, 서로 다른 버퍼를 참조하기 때문이다.(수신버퍼,송신버퍼))
따라서 확장까지 고려한다면, 같은 함수가 여러 쓰레드에서 동시에 호출될 경우는 문제가 될 수 있음. 이때는 동기화를 해야한다. 같은 버퍼를 참조하기 때문이다.
3. 에러 뜨니까 if문으로 디버깅
4. 받을 때까지 대기하게 만들어야함. -> recv() 함수는 기본적으로 블록킹 동작을 함.(응답이 올때까지 대기함.)
5. 서로 독립적인 쓰레드이다. pthreads 라이브러리는 커널레벨이다.
6. perror은 단순 에러출력, exit()은 프로세스 종료.
*/

void *receiveFromClient(void *arg) {
    char buf[BUFSIZE+1];
    while (1) {
        ssize_t received = recvfrom(clientSock, buf, BUFSIZE, 0, (struct sockaddr *)&clientAddr, &clientAddrLen);
        if (received > 0) {
            buf[received] = '\0';
            printf("Client: %s", buf);
        } else {
            perror("receive failed");
            break;
        }
    }
    return NULL;
}
void *sendToClient(void *arg) {
    char buf[BUFSIZE];
    while (1) {
        if (fgets(buf, BUFSIZE, stdin) == NULL) {    // fgets가 NULL을 반환하면 EOF(리눅스에서는: Ctrl+D)로 설정
            printf("Server: This host left this chatting room.");
            fflush(stdout);  //printf를 해도 출력되지 않아서, 버퍼를 수동으로 플러시해야한다.  
            strcpy(buf, "Server: This host left this chatting room.\n");
            if (sendto(clientSock, buf, strlen(buf), 0, (struct sockaddr *)&clientAddr, clientAddrLen) < 0) {
                perror("send failed");
            }   
            break; // 서버가 채팅을 종료하려고 할 때 루프를 빠져나옵니다.
        }
        
        if (strlen(buf) > 1) { // fgets는 항상 '\n'을 포함하므로, 최소 길이는 1 보다큼.
            if (sendto(clientSock, buf, strlen(buf), 0, (struct sockaddr *)&clientAddr, clientAddrLen) < 0) {
                perror("send failed");
                break;
            }
            printf("Server: %s", buf);
        }
    }
    return NULL;
}

int main() {
    // 1. create socket
    clientSock = socket(AF_INET,SOCK_DGRAM,0);
    if (clientSock < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    // server address , memset을 이용한 서버주소 초기화
    struct sockaddr_in serveraddr;
    memset(&serveraddr,0,sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);

    // 2. bind
    if (bind(clientSock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("host join this chatting room.\n");

    // 쓰레드 2개 생성(pid)
    pthread_t threadRecv, threadSend;
    pthread_create(&threadRecv, NULL, receiveFromClient,NULL);
    pthread_create(&threadSend, NULL, sendToClient,NULL);
    
    // 스레드 종료시 리소스 해제(join)
    pthread_join(threadRecv, NULL);
    pthread_join(threadSend, NULL);

    close(clientSock);
    return 0;
}