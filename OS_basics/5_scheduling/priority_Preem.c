#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// 프로세스 구조체 정의
typedef struct Process {
    int process_id;      // 프로세스 ID
    int arrival_time;    // 도착 시간
    int burst_time;      // 실행 시간 (초)
    int remaining_time;  // 남은 실행 시간
    int priority;        // 우선순위
    int active;          // 프로세스 활성 상태
    int executed_time;   // 실행된 시간 추적
    struct Process *next; // 다음 프로세스
} Process;

// 간트 차트를 위한 인터벌 구조체 정의
typedef struct Interval {
    int process_id;    // 프로세스 ID
    int start_time;    // 시작 시간
    int end_time;      // 종료 시간
    struct Interval *next; // 다음 인터벌
} Interval;

// 프로세스와 인터벌을 위한 링크드 리스트 구조체 정의
typedef struct {
    Process *head;    // 리스트의 첫 번째 프로세스
    Process *tail;    // 리스트의 마지막 프로세스
} ProcessList;

typedef struct {
    Interval *head;   // 리스트의 첫 번째 인터벌
    Interval *tail;   // 리스트의 마지막 인터벌
} IntervalList;

// 간트 차트 구조체 정의
typedef struct Gant {
    int process_id;   // 프로세스 ID
} Gant;

ProcessList readyQueue;
IntervalList workingQueue;
Gant gant[62];
pthread_mutex_t mutex;  // 상호배제를 위한 뮤텍스
pthread_cond_t cond;    // 조건 변수
int current_time = 0;   // 현재 시간
Process *active_pkt = NULL;     // 활성 프로세스

// 프로세스 리스트 초기화
void initProcessList(ProcessList *list) {
    list->head = list->tail = NULL;
}

// 인터벌 리스트 초기화
void initIntervalList(IntervalList *list) {
    list->head = list->tail = NULL;
}

// 우선순위에 따라 프로세스를 프로세스 리스트에 삽입
void enqueue(ProcessList *list, Process *newProcess) {
    if (list->head == NULL) {
        list->head = list->tail = newProcess;
    } else {
        Process *current = list->head, *prev = NULL;
        while (current != NULL && current->priority < newProcess->priority) {
            prev = current;
            current = current->next;
        }
        if (prev == NULL) {
            newProcess->next = list->head;
            list->head = newProcess;
        } else {
            newProcess->next = current;
            prev->next = newProcess;
            if (current == NULL) {
                list->tail = newProcess;
            }
        }
    }
}

// 프로세스 리스트에서 프로세스를 제거
void removeProcess(ProcessList *list, int process_id) {
    if (list->head == NULL) return;

    Process *current = list->head;
    Process *prev = NULL;

    while (current != NULL) {
        if (current->process_id == process_id) {
            if (prev == NULL) {
                list->head = current->next;
                if (list->head == NULL) {
                    list->tail = NULL;
                }
            } else {
                prev->next = current->next;
                if (current->next == NULL) {
                    list->tail = prev;
                }
            }
            return;
        }
        prev = current;
        current = current->next;
    }
}

// 새로운 인터벌 생성
Interval* createInterval(int process_id, int start_time, int end_time) {
    Interval *newInterval = (Interval*)malloc(sizeof(Interval));
    newInterval->process_id = process_id;
    newInterval->start_time = start_time;
    newInterval->end_time = end_time;
    newInterval->next = NULL;
    return newInterval;
}

// 인터벌 리스트에 인터벌 추가
void addInterval(IntervalList *list, int process_id, int start_time, int end_time) {
    Interval *newInterval = createInterval(process_id, start_time, end_time);
    if (list->head == NULL) {
        list->head = list->tail = newInterval;
    } else {
        list->tail->next = newInterval;
        list->tail = newInterval;
    }
}

// 인터벌 리스트 출력
void printIntervals(IntervalList *list) {
    Interval *current = list->head;
    while (current != NULL) {
        printf("P%d (%d-%d)\n", current->process_id, current->start_time, current->end_time);
        current = current->next;
    }
}

// 프로세스 리스트 출력
void printProcessList(ProcessList *list) {
    Process *current = list->head;
    while (current != NULL) {
        printf("P%d: Arrival Time = %d, Burst Time = %d, Remaining Time = %d, Priority = %d\n", 
                current->process_id, current->arrival_time, current->burst_time, current->remaining_time, current->priority);
        current = current->next;
    }
}

// 작업 함수: 주어진 프로세스의 실행 내용을 출력하고 실행된 시간을 증가시킴
void working_func(int process_id, int start, int end, int *executed_time) {
    for (int i = start; i < end; i++) {
        printf("P%d: %d * %d = %d\n", process_id, process_id, *executed_time + 1, process_id * (*executed_time + 1));
        (*executed_time)++;
    }
}

// 스레드 함수: 각 프로세스를 실행
void* process_thread(void* arg) {
    while (1) {
        pthread_mutex_lock(&mutex);

        // 현재 시간에 도착한 가장 높은 우선순위의 프로세스를 찾기
        Process *p = readyQueue.head;
        Process *arrival_pkt = NULL;

        while (p != NULL) {
            if (current_time >= p->arrival_time && p->remaining_time > 0) {
                if (!arrival_pkt || p->priority > arrival_pkt->priority) {
                    arrival_pkt = p;
                }
                // 우선순위가 같은 경우, 도착 시간이 더 빠른 프로세스를 선택
                if (p->priority == arrival_pkt->priority) {
                    if (p->arrival_time < arrival_pkt->arrival_time) {
                        arrival_pkt = p;
                    }
                }
            }
            p = p->next;
        }

        // 새로 도착한 프로세스가 더 높은 우선순위를 가지면 프로세스를 교체
        if (arrival_pkt && (!active_pkt || arrival_pkt->priority > active_pkt->priority)) {
            active_pkt = arrival_pkt;
        }

        // 새로 도착한 프로세스가 없으면, 가장 높은 우선순위의 프로세스를 찾기
        if (arrival_pkt == NULL) {
            Process *max_priority_process = NULL;
            p = readyQueue.head;
            while (p != NULL) {
                if (max_priority_process == NULL || p->priority > max_priority_process->priority) {
                    max_priority_process = p;
                }
                if (max_priority_process == NULL || p->priority == max_priority_process->priority) {
                    if (p->arrival_time < max_priority_process->arrival_time) {
                        max_priority_process = p;
                    }
                }
                p = p->next;
            }
            active_pkt = max_priority_process;
        }

        // 간트 차트에 기록
        if (active_pkt && active_pkt->remaining_time > 0) {
            gant[current_time].process_id = active_pkt->process_id;
            active_pkt->remaining_time--;

            // 작업 실행
            working_func(active_pkt->process_id, current_time, current_time + 1, &active_pkt->executed_time);

            // 남은 시간이 0이면 준비 큐에서 프로세스를 제거
            if (active_pkt->remaining_time == 0) {
                removeProcess(&readyQueue, active_pkt->process_id);
                active_pkt = NULL;
            }
        } 
        else {
            gant[current_time].process_id = 0; // 실행 중인 프로세스가 없음
        }

        pthread_mutex_unlock(&mutex);
        usleep(100000); // 0.1초 대기
        current_time++;

        if (current_time >= 62) {
            break;
        }
    }

    return NULL;
}

int main() {
    // 동기화를 위한 뮤텍스
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    // 프로세스 정의
    Process processes[5] = {
        {1, 0, 10, 10, 3, 2, 0, NULL},
        {2, 1, 28, 28, 2, 2, 0, NULL},
        {3, 2, 6, 6, 4, 2, 0, NULL},
        {4, 3, 4, 4, 1, 2, 0, NULL},
        {5, 4, 14, 14, 2, 2, 0, NULL}
    };

    // 준비 큐 초기화
    initProcessList(&readyQueue);

    // 준비 큐에 프로세스 삽입
    for (int i = 0; i < 5; i++) {
        enqueue(&readyQueue, &processes[i]);
    }

    // 프로세스 리스트 출력
    printf("Initial Process List:\n");
    printProcessList(&readyQueue);

    // 스레드 생성 및 실행
    pthread_t thread;
    pthread_create(&thread, NULL, process_thread, NULL);

    // 스레드 종료 대기
    pthread_join(thread, NULL);

    // 간트 차트를 이용해 인터벌 리스트 생성
    initIntervalList(&workingQueue);

    int current_process = gant[0].process_id;
    int start_time = 0;
    
    for (int i = 1; i < 62; i++) {
        if (gant[i].process_id != current_process) {
            addInterval(&workingQueue, current_process, start_time, i);
            current_process = gant[i].process_id;
            start_time = i;
        }
    }
    // 마지막 인터벌 추가 (간트 차트를 기반으로 인터벌 리스트를 생성할 때, 마지막 프로세스의 실행 구간을 포함하기 위해서)
    addInterval(&workingQueue, current_process, start_time, 62);

    // 인터벌 리스트 출력
    printIntervals(&workingQueue);

    // 반환 시간 및 대기 시간 계산
    int total_turnaround_time = 0;
    int total_waiting_time = 0;

    Interval *current = workingQueue.head;
    while (current != NULL) {
        for (int i = 0; i < 5; i++) {
            if (processes[i].process_id == current->process_id) {
                processes[i].executed_time = current->end_time - current->start_time;
                break;
            }
        }
        current = current->next;
    }

    for (int i = 0; i < 5; i++) {
        int finish_time = 0;
        current = workingQueue.head;
        while (current != NULL) {
            if (current->process_id == processes[i].process_id) {
                if (current->end_time > finish_time) {
                    finish_time = current->end_time;
                }
            }
            current = current->next;
        }
        int turnaround_time = finish_time - processes[i].arrival_time;
        int waiting_time = turnaround_time - processes[i].burst_time;

        total_turnaround_time += turnaround_time;
        total_waiting_time += waiting_time;

        printf("P%d: Turnaround Time = %d, Waiting Time = %d\n", processes[i].process_id, turnaround_time, waiting_time);
    }

    // 평균 반환 시간 및 평균 대기 시간 출력
    printf("Average Turnaround Time = %.2f\n", (float)total_turnaround_time / 5);
    printf("Average Waiting Time = %.2f\n", (float)total_waiting_time / 5);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}
