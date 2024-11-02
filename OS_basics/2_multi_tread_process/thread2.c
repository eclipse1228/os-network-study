#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>

// thread에서 사용하는 함수들의 시그니처 수정
void* f1(void* arg) {
    int n = *(int*)arg; // arg로 받은 포인터를 int로 변환
    for(int i = 1; i <= 25; i++){
        printf("%d ", i * n);
    }
    printf("\n");
    return NULL; // void* 반환 타입에 맞게 NULL 반환
}

void* f2(void* arg) {
    int n = *(int*)arg;
    for(int i = 26; i <= 50; i++){
        printf("%d ", i * n);
    }
    printf("\n");
    return NULL;
}

void* f3(void* arg) {
    int n = *(int*)arg;
    for(int i = 51; i <= 75; i++){
        printf("%d ", i * n);
    }
    printf("\n");
    return NULL;
}

void* f4(void* arg) {
    int n = *(int*)arg;
    for(int i = 76; i <= 100; i++){
        printf("%d ", i * n);
    }
    printf("\n");
    return NULL;
}

void mulp(int n) {
    pthread_t tid1, tid2, tid3, tid4;

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
    int n = 3; // 곱셈에 사용할 숫자 시작값

    // 프로세스 생성 로직 수정
    for (int i = 0; i < 4; i++) {
        pid = fork();
        if (pid == 0) { // 자식 프로세스
            // n 값을 조정하여 각 프로세스에서 다른 작업을 수행
            mulp(n + i*2);
            return 0; // 자식 프로세스 종료
        }
    }

    // 부모 프로세스에서 자식 프로세스들이 모두 종료되기를 기다림
    for (int i = 0; i < 4; i++) {
        wait(NULL);
    }

    return 0;
}
