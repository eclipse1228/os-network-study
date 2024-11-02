#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

int main() {
    struct timespec start, end; // 시작 , 종료 시간
    double time; // 프로세스 개별 실행 시간 
    pid_t pid;
    int n = 1; // 각 프로세스의 작업 범위 시작점
    clock_gettime(CLOCK_MONOTONIC, &start);

    // 첫 번째 fork: 프로세스를 2개로 분기
    if ((pid = fork()) == 0) {
        n += 500; 
    }

    // 두 번째 fork: 프로세스를 4개로 분기
    if ((pid = fork()) == 0) {
        n += 250; 
    }

    // 세 번째 fork: 프로세스를 8개로 분기
    if ((pid = fork()) == 0) {
        n += 125; 
    }

    // 각 프로세스의 작업 실행
    for (int i = n; i < n + 125; i++) {
        printf("%d ", i * 7);
    }

    printf("\n");

    // 초단위 시간 측정 및 출력
    clock_gettime(CLOCK_MONOTONIC, &end); 
    time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / (double)1000000000L;
    printf("<<Pid: %d, start: %d, 실행 시간: %lf 초>>\n", getpid(), n, time);

    return 0;
}
