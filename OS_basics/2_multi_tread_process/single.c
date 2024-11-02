#include<stdio.h>
#include<sys/time.h>
#include<time.h>
#include<unistd.h> // getpid() 함수 사용을 위해 추가

int main() {
    struct timespec start, end; // 시작, 종료 시간
    double time; // 프로세스 개별 실행 시간

    // 시작 시간 측정
    clock_gettime(CLOCK_MONOTONIC, &start);

    for(int i = 3; i <= 9; i += 2) {
        for(int j = 1; j <= 100; j++) {
            printf("%d ", j * i);
        }
        printf("\n");
    }

    // 종료 시간 측정 및 실행 시간 계산
    clock_gettime(CLOCK_MONOTONIC, &end);
    time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("<<Pid: %d, 실행 시간: %lf 초>>\n", getpid(), time);
}