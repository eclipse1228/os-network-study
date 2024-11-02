#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// 프로세스 구조체
typedef struct Process {
    int process_id;
    int exe_count;          // 실행된 횟수
    int burst_time;         // 실행 시간 (초)
    int remaining_time;     // 남은 실행 시간
    int turnAround_time;    // 반환시간
    int wait_time;          // 대기시간
    struct Process *next;   // 다음 프로세스 포인터
} Process;

// 간트 차트 구조체
typedef struct Gant {
    int id;                 // 프로세스 ID
    int start_time;         // 시작 시간
    int end_time;           // 종료 시간
    struct Gant *next;      // 다음 간트 차트 포인터
} Gant;

// 큐 구조체
typedef struct Circular_Queue {
    Process *head;          // 큐의 시작 포인터
    Process *tail;          // 큐의 끝 포인터
    pthread_mutex_t mutex;  // 큐별 뮤텍스
    pthread_cond_t cond;    // 큐별 조건 변수
    Gant *gant_head;        // 간트 차트 시작 포인터
    Gant *gant_tail;        // 간트 차트 끝 포인터
} Circular_Queue;

int current_time = 0;   // 현재 시간 추적을 위한 변수
int p_count = 3;        // 남은 프로세스 개수
int timing = 1;         // 전역 타이밍 변수 (==누구 차례)

// 큐 3개 초기화
Circular_Queue queue1 = {NULL, NULL, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, NULL, NULL};
Circular_Queue queue2 = {NULL, NULL, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, NULL, NULL};
Circular_Queue queue3 = {NULL, NULL, PTHREAD_MUTEX_INITIALIZER, PTHREAD_COND_INITIALIZER, NULL, NULL};

pthread_mutex_t global_mutex = PTHREAD_MUTEX_INITIALIZER; // 전역 뮤텍스
pthread_cond_t timing_cond = PTHREAD_COND_INITIALIZER; // 타이밍 조건 변수

void enqueue(Circular_Queue *queue, Process *process) {
    pthread_mutex_lock(&queue->mutex); // 큐의 뮤텍스 잠금
    if (queue->tail == NULL) {
        queue->head = queue->tail = process;
    } else {
        queue->tail->next = process;
        queue->tail = process;
    }
    process->next = NULL;
    pthread_cond_signal(&queue->cond); // 조건 변수 신호
    pthread_mutex_unlock(&queue->mutex); // 큐의 뮤텍스 잠금 해제
}

// 간트를 순환 큐에 넣기
void log_gant(Circular_Queue *queue, int process_id, int start_time, int end_time) {
    Gant *gant_entry = (Gant *)malloc(sizeof(Gant));
    gant_entry->id = process_id;
    gant_entry->start_time = start_time;
    gant_entry->end_time = end_time;
    gant_entry->next = NULL;

    if (queue->gant_tail == NULL) {                             // 비었다면, 
        queue->gant_head = queue->gant_tail = gant_entry;       // 하나만 있다
    } else {
        queue->gant_tail->next = gant_entry;                    // gant_entry 는 추가 될 entry
        queue->gant_tail = gant_entry;
    }
}

// 큐로 간트 출력
void print_queue_gant(Circular_Queue *queue, int queue_number) {
    printf("Q%d: ", queue_number);
    Gant *current = queue->gant_head;           // head 위치로
    while (current != NULL) {                   // 전체 큐 출력 
        printf("P%d (%d-%d) ", current->id, current->start_time, current->end_time);
        current = current->next;
    }
    printf("\n");
}

// 쓰레드 함수
void *process_thread(void *arg) {
    Process *process = (Process *)arg;
    int my_timing = process->process_id;            // 내 차례 변수
    int time_slice = 1;                             // 큐1 부터 시작
    Circular_Queue *current_queue = &queue1;        
    
    while (process->remaining_time > 0) {

        pthread_mutex_lock(&global_mutex);          // lock 걸기

        // 내 차례가 올 때까지 대기
        while (timing != my_timing) {
            pthread_cond_wait(&timing_cond, &global_mutex); 
        }

        if(p_count == 1) {                          // process가 1개 남았는가 ? -> 나머지는 큐 1에서 진행
            time_slice = 1;
            current_queue = &queue1;
        }
        
        /* 큐 실행 */
        for (int i = 0; i < time_slice; i++) {
            printf("P%d: %d * %d = %d\n", process->process_id, process->exe_count, process->process_id, process->exe_count * process->process_id);
            process->exe_count++;
        }
        
        if(process->remaining_time < 2) {           // 남은 시간이 2 미만입니까 ? -> 큐 1 (예제의 마지막 p2의 Q1으로 이동)
            current_queue = &queue1;
        }

        /* 선형 큐에 삽입 , 프로세스 남은시간 ,전역변수 업데이트, */
        log_gant(current_queue, process->process_id, current_time, current_time + time_slice);
        process->remaining_time -= time_slice;
        current_time += time_slice;
        
        /* Q별 time_slice (정해진 시간) 조건 */
        if (process->remaining_time > 0) {
            if (time_slice == 1) {                  // Q1 입니까?
                current_queue = &queue2;
                time_slice = 2;
            } else if (time_slice == 2) {           // Q2 입니까?
                current_queue = &queue3;
                time_slice = 4;
            } else if (time_slice == 4) {           // Q3 입니까?
                if (process->remaining_time > 2 && process->remaining_time >= 4 && p_count != 1) {
                        time_slice = 4;
                        current_queue = &queue3;
                    } 
                    else if (process->remaining_time > 1 && process->remaining_time >= 4 && p_count != 1) {
                        current_queue = &queue2;
                        time_slice = 2;
                    } 
                    else if (process->remaining_time > 1 && p_count == 1) {
                        current_queue = &queue1;
                        time_slice = 1;
                    } 
                    else if (process->remaining_time < 4) {
                        time_slice = process->remaining_time;
                    } 
                    else {
                        current_queue = &queue1;
                        time_slice = 1;
                    }
            }
        }
        /*큐에 프로세스 삽입 (log_gant와는 다름 (log_gant는 문제의 조건이 원하는 출력을 위해 만들어낸 gant를 넣은 큐)*/ 
        enqueue(current_queue, process);

        /* p_count 는 실행중인 프로세스의 개수, 개수에 따라 timing을 수정한다. 3종료후 2 부터시작하는 예외 제거*/
        if (p_count == 2) {
            if (timing == 1) {
                timing = 2;
            } 
            else if (timing == 2) {
                timing = 1;
            }
        } 
        else if (p_count == 1) {
            if (timing == 1) {
                timing = 1;
            } 
            else if (timing == 2) {
                timing = 2;
            }
        } 
        else {
            timing = (timing % 3) + 1;
        }

        /* 남은 시간 다 소비, -> p_count 1 감소.*/
        if (process->remaining_time <= 0) {
            process->turnAround_time = current_time;
            p_count--;
        }

        pthread_cond_broadcast(&timing_cond); // 다음 스레드에게 신호
        pthread_mutex_unlock(&global_mutex);
        sleep(1);
    }

    return NULL;
}

int main() {
    pthread_t process_threads[3]; // 쓰레드 배열

    // 초기 프로세스 생성 및 큐에 추가
    Process p1 = {1, 1, 30, 30, 0, 0, NULL};
    Process p2 = {2, 1, 20, 20, 0, 0, NULL};
    Process p3 = {3, 1, 10, 10, 0, 0, NULL};

    // 프로세스 스레드 생성
    pthread_create(&process_threads[0], NULL, process_thread, (void *)&p1);
    pthread_create(&process_threads[1], NULL, process_thread, (void *)&p2);
    pthread_create(&process_threads[2], NULL, process_thread, (void *)&p3);

    // 프로세스 스레드 종료 대기
    for (int i = 0; i < 3; i++) {
        pthread_join(process_threads[i], NULL);
    }

    printf("\n");
    // 큐 상태 출력 (간트 배열을 출력함)
    print_queue_gant(&queue1, 1);
    print_queue_gant(&queue2, 2);
    print_queue_gant(&queue3, 3);

    // 반환 시간 및 대기 시간 계산 및 출력
    int total_turnaround_time = 0;
    int total_wait_time = 0;

    // 프로세스 3개
    Process *processes[] = {&p1, &p2, &p3};
    printf("\n");

    // 반환,대기시간 출력
    for (int i = 0; i < 3; i++) {
        processes[i]->wait_time = processes[i]->turnAround_time - processes[i]->burst_time;
        total_turnaround_time += processes[i]->turnAround_time;
        total_wait_time += processes[i]->wait_time;
        printf("P%d Turnaround Time: %d, Waiting Time: %d\n", processes[i]->process_id, processes[i]->turnAround_time, processes[i]->wait_time);
    }

    printf("Average Turnaround Time: %.2f, Average Waiting Time: %.2f\n", total_turnaround_time / 3.0, total_wait_time / 3.0);

    return 0;
}
/*
mlfq를 구현하기 위해 멀티 쓰레드를 사용하였습니다.
순환큐 구조의 큐를 사용하였으며, 상호배제를 위해 pthread 라이브러리의 mutex_lock, 쓰레드별 순서 동기화를 위해 pthread의 wait, broadcast와 전역변수로
timing (1,2,3) 을 설정하여 순서를 동기화시켰습니다. 
저는 문제에서 주어진 Q1,Q2,Q3를 출력하기 위해서 간트차트 구조체를 만들고, 구조체를 큐안에 넣었습니다. 왜냐하면 간트차트에 실행시간,종료시간을 넣어 사용하는데,
문제에서도 각 프로세스의 실행시간과 종료시간을 원했기 때문입니다. 때문에 출력시 문제처럼 Q1이 2개가 나올 수 없었고, 마지막 Q1의 P1(53-60)도
P1(53-54) P1(54-55).. 처럼 출력된 것입니다. 제가 이해한 mlfq에 따르면 전체 큐에 프로세스가 하나 남은 상황에서는 Q1에서 전부 실행한다고 알고있기 때문에
P1(53-60) 과 같은 표현은 마치 7실행시간을 가진 것 처럼 보이기 때문에 제 표현이 더 나아보였기 떄문에 수정하지 않았습니다.

라운드 로빈과 비교해서 반환시간과 대기시간은 이렇습니다. mlfq가 평균 대기시간이 줄었습니다. 
동적으로 큐를 3개 사용함으로써 사용률이 증가했다고 볼 수 있습니다.
*/