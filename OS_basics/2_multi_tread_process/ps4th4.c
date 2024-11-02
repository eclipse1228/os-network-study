#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

// 쓰레드의 넘버를 부여한다.
int collectInt(int x) { 
    int y;
    switch(x) {
        case 3: y = 1; break;
        case 5: y = 2; break;
        case 7: y = 3; break;
        case 9: y = 4; break;
    }
    return y;
}

void* f1(void* arg) {
    int n = *(int*)arg;
    int PsNum = collectInt(n);
    printf("<Process %d :Thread 1>",PsNum);
    for(int i = 1; i <= 25; i++){
        printf("%d ", i * n);
    }
    printf("\n");
    return NULL;
}

void* f2(void* arg) {
    int n = *(int*)arg;
    int PsNum = collectInt(n);
    printf("<Process %d :Thread 2>",PsNum);
    for(int i = 26; i <= 50; i++){
        printf("%d ", i * n);
    }
    printf("\n");
    return NULL;
}

void* f3(void* arg) {
    int n = *(int*)arg;
    int PsNum = collectInt(n);
    printf("<Process %d :Thread 3>",PsNum);
    for(int i = 51; i <= 75; i++){
        printf("%d ", i * n);
    }
    printf("\n");
    return NULL;
}

void* f4(void* arg) {
    int n = *(int*)arg;
    int PsNum = collectInt(n);
    printf("<Process %d :Thread 4>",PsNum);
    for(int i = 76; i <= 100; i++){
        printf("%d ", i * n);
    }
    printf("\n");
    return NULL;
}

void mulp(int n) {
    pthread_t tid1, tid2, tid3, tid4;

    if(n==3) printf("<Process 1>\n");
    else if(n==5) printf("<Process 2>\n");
    else if(n==7) printf("<Process 3>\n");
    else if(n==9) printf("<Process 4>\n");
    // 함수는 f1~f4까지 부여
    pthread_create(&tid1, NULL, f1, &n);
    pthread_create(&tid2, NULL, f2, &n);
    pthread_create(&tid3, NULL, f3, &n);
    pthread_create(&tid4, NULL, f4, &n);
    // join으로 
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    pthread_join(tid4, NULL);
}

int main() { 
    int pid;
    // n
    int n = 3;
    double time;

    struct timespec start, end;

    // 시간 재기 시작. 
    clock_gettime(CLOCK_MONOTONIC, &start);

    // 프로세스 4개 생성
    if(pid = fork() == 0) {
        n += 4;
    } 
    
    if(pid = fork() == 0) {
        n += 2;
    }
    // 쓰레드 생성 및 계산하는 함수 mulp() , join()까지.
    mulp(n);

    // 시간 종료
    clock_gettime(CLOCK_MONOTONIC, &end);
    time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("<<Pid: %d, 실행 시간: %lf 초>>\n", getpid(), time);
}