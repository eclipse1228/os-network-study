#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVERPORT 9000 // 서버 포트
#define BUFSIZE 1024

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

char *SERVERIP = (char *)"127.0.0.1"; // 서버 IP 주소 (로컬)
int sock;  // 전역으로 선언 

// 서버로 보내는 쓰레드 함수
void *sendToServer(void *arg) { 
    char buf[BUFSIZE+1];
    while (1) {
        fgets(buf, BUFSIZE, stdin);
        if (send(sock, buf, strlen(buf), 0) < 0) {
            perror("send failed");
            break;
        }
        printf("Client: %s",buf);
    }
    return NULL;
}
// 서버로 부터 받는 쓰레드 함수
void *receiveFromServer(void *arg) {
    char buf[BUFSIZE+1];
    while (1) {
        ssize_t received = recv(sock, buf, BUFSIZE, 0); // 받을 때 까지 대기함... !
        if (received > 0) {
            buf[received] = '\0';
            printf("Server: %s", buf);
        } 
        else {
            if (received == 0) {
                printf("Server: This host left this chatting room.\n");
            } 
            else {
                perror("receive failed");
            }
            break;
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {

    // 0. 소켓 선언
    sock = socket(AF_INET, SOCK_STREAM, 0);  
    if (sock < 0) { 
        perror("socket creation failed");
        exit(EXIT_FAILURE);                
    }
    // 1. 소켓 주소 
    struct sockaddr_in serveraddr;          // 소켓 주소  
    memset(&serveraddr, 0, sizeof(serveraddr)); // 주소 초기화
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(SERVERPORT);

    // ip 주소를 네트워크 체계로 and ServerIP 연결 
    if (inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr) <= 0) {
        perror("inet_pton failed");
        exit(EXIT_FAILURE);
    }

    // 2. connect
    if (connect(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    printf("host join this chatting room.\n");
    
    // 쓰레드 2개 생성(pid)
    pthread_t thread_send, thread_recv;     
    pthread_create(&thread_send, NULL, sendToServer, NULL);
    pthread_create(&thread_recv, NULL, receiveFromServer, NULL);

    // 스레드 종료시 리소스 해제(join)
    pthread_join(thread_send, NULL);
    pthread_join(thread_recv, NULL);

    close(sock); // 소켓 닫기 
    return 0;
}

