#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
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

// thread에서 사용하는 함수들의 시그니처 수정
void* f1(void* arg) {
    int n = *(int*)arg; // arg로 받은 포인터를 int로 변환
    int PsNum = collectInt(n);
    printf("<Process %d :Thread 1>",PsNum);

    for(int i = 1; i <= 25; i++){
        printf("%d ", i * n);
    }
    printf("\n");
    return NULL; // void* 반환 타입에 맞게 NULL 반환
}

void* f2(void* arg) {
    int n = *(int*)arg;
    int PsNum = collectInt(n);
    printf("<Process %d :Thread 1>",PsNum);

    for(int i = 26; i <= 50; i++){
        printf("%d ", i * n);
    }
    printf("\n");
    return NULL;
}

void* f3(void* arg) {
    int n = *(int*)arg;
    int PsNum = collectInt(n);
    printf("<Process %d :Thread 1>",PsNum);

    for(int i = 51; i <= 75; i++){
        printf("%d ", i * n);
    }
    printf("\n");
    return NULL;
}

void* f4(void* arg) {
    int n = *(int*)arg;
    int PsNum = collectInt(n);
    printf("<Process %d :Thread 1>",PsNum);

    for(int i = 76; i <= 100; i++){
        printf("%d ", i * n);
    }
    printf("\n");
    return NULL;
}

void mulp(int n) {
    pthread_t tid1, tid2, tid3, tid4;

    if(n==3) 
    printf("<Process 1>\n");

    else if(n==5) 
    printf("<Process 2>\n");

    else if(n==7)
    printf("<Process 3>\n");

    else if(n==9)
    printf("<Process 4>\n");

    // 쓰레드 생성시 사용법 수정: 함수 이름 뒤의 괄호 제거, 인자 전달 방식 수정
    pthread_create(&tid1, NULL, f1, &n);
    pthread_create(&tid2, NULL, f2, &n);
    pthread_create(&tid3, NULL, f3, &n);
    pthread_create(&tid4, NULL, f4, &n);

    // 모든 스레드가 종료될 때까지 대기
    pthread_join(tid1, NULL);
    pthread_join(tid2, NULL);
    pthread_join(tid3, NULL);
    pthread_join(tid4, NULL);
}


int main() { 
    int pid;
    // n
    int n = 3;

    // create Process for 4
    // first fork
    if(pid = fork() == 0) {
        n += 4;
    } 
    
    if(pid = fork() == 0) {
        n += 2;
    }

    mulp(n);
    

}