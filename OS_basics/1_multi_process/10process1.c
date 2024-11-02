#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/wait.h>

int main() {
    double time;
    struct timespec start, end; // 시작 , 종료 시간
    clock_gettime(CLOCK_MONOTONIC, &start); 

    for (int i = 0; i < 10; i++) {
        pid_t pid = fork();

        if (pid == 0) {
            int start = i * 100 + 1;
            int end = (i + 1) * 100;

             for (int i = start; i <= end; i++) {
                printf("%d ", i * 10);
            }
            printf("\n <<Pid: %d,start: %d>> \n",getpid(),start);

            _exit(0); 
        } else if (pid == -1) {
            perror("fork failed");
            return 1;
        }
    }

    for (int i = 0; i < 10; i++) {
        wait(NULL); 
        // NULL 을 넣으면, 종료되기를 기다리는게 아니라, 종료상태필요없이 실행 완료되기만을 기다린다. NULL은 종료상태 NULL 관심없다는 의미.
    }

    clock_gettime(CLOCK_MONOTONIC, &end); 
    time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / (double)1000000000L;
    printf("<multiProcess Time>: %lf \n", time);

    return 0;
}
