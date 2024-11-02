#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

// 프로세스
typedef struct Process {
    int process_id;
    int arrival_time; // 도착시간
    int burst_time; // 실행 시간 (초)
    int remaining_time; // 남은 실행 시간
    int priority; // 우선순위
    int active; // 프로세스 활성 상태 
    int latest_start_time; // 마지막 시작시간
    int turnAround_time; // 반환시간
    int wait_time; // 대기시간
    struct Process *next; // 다음 프로세스
} Process;

// 간트 
typedef struct Gant {
    int id;
    int start_time;
    int end_time;
} Gant;

int current_time = 0;   // 현재 시간 추적을 위한 공유변수
Gant gant[14];          // 간트 14개 
pthread_mutex_t mutex;  // 상호배제를 위한 뮤텍스
pthread_cond_t cond;    // 상호배제를 위한 뮤텍스
int current_index = 0;  // 간트를 위한 공유 변수 

typedef struct Circular_Queue {
    Process *head;
    Process *tail;
} Circular_Queue;

/* 큐에 넣기*/
void push_to_Q(Circular_Queue* q, Process p) { 
    // 새로운 Process 노드 생성
    Process* new_process = (Process*)malloc(sizeof(Process));
    *new_process = p;
    new_process->next = NULL;

    if (q->head == NULL) {
        // 큐가 비어있는 경우
        q->head = new_process;
        q->tail = new_process;
        new_process->next = new_process; // 원형 큐이므로 자기 자신을 가리킴
    } else {
        // 큐가 비어있지 않은 경우
        new_process->next = q->head; // 새로운 노드가 head를 가리키도록
        q->tail->next = new_process; // 현재 tail의 다음 노드가 새로운 노드를 가리키도록
        q->tail = new_process; // tail을 새로운 노드로 갱신
    }
}
/* 큐에서 뺴기 */
Process* pop_from_Q(Circular_Queue* q) {
    if (q->head == NULL) {
        return NULL; // 큐가 비어있는 경우
    }

    Process* removed_process = q->head;
    if (q->head == q->tail) {
        // 큐에 노드가 하나만 있는 경우
        q->head = NULL;
        q->tail = NULL;
    } else {
        q->head = removed_process->next;
        q->tail->next = q->head;
    }
    removed_process->next = NULL; // 제거된 노드의 next 포인터 초기화
    return removed_process;
}

/* 실행 함수 */
void exe_func(Process* p, int count) {
    //  곱하는 수는 (실행시간(burst_time) - 남은시간(remaining_time))
    for(int i = 1; i <= count; i++) { 
        printf("P%d: %d * %d\n", p->process_id, p->process_id, p->burst_time - (p->remaining_time - i));
    }
}
/* 큐의 끝에 넣기 함수 */
void move_head_to_tail(Circular_Queue* q, int i) {
    pthread_mutex_lock(&mutex);

    if (q->head == NULL) {
        pthread_mutex_unlock(&mutex);
        return; // 큐가 비어있는 경우
    }

    Process* first_process = pop_from_Q(q);

    if (first_process->remaining_time <= 5) {
        // 실행, remaining_time 만큼
        exe_func(first_process, first_process->remaining_time);

        // gant 저장
        gant[i].id = first_process->process_id;
        gant[i].start_time = current_time;
        gant[i].end_time = current_time + first_process->remaining_time;

        current_time += first_process->remaining_time;
        free(first_process); // 동적으로 할당된 메모리 해제
    } 
    
    else {
        // 실행, 5 만큼
        exe_func(first_process, 5);
        first_process->remaining_time -= 5;

        gant[i].id = first_process->process_id;
        gant[i].start_time = current_time;
        gant[i].end_time = current_time + 5;

        current_time += 5;
        push_to_Q(q, *first_process);
        free(first_process); // 동적으로 할당된 메모리 해제
    }

    pthread_mutex_unlock(&mutex);
}

void* process_thread(void* arg) {
    Circular_Queue* q = (Circular_Queue*)arg;

    while (current_index < 14) {
        move_head_to_tail(q, current_index);    // 큐 처리
        current_index++;                        // 간트를 위한 index 증가
        usleep(100);                            // 조금 기다리기
    }

    return NULL;
}

/* 간트 출력*/
void print_gant_chart(Gant* gant, int size) {
    printf("\nGantt Chart:\n");
    for (int i = 0; i < size; i++) {
        if (gant[i].id != 0) {
            printf("P%d(%d-%d)\n", gant[i].id, gant[i].start_time, gant[i].end_time);
        }
    }
    printf("\n");
}

int main() { 
    // 공유변수 상호배제 동기화를 위한 뮤텍스 
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);

    // 프로세스 입력
    Process processes[5] = {
        {1, 0, 10, 10, 3, 1, 0, 0, 0, NULL},
        {2, 1, 28, 28, 2, 1, 0, 0, 0, NULL},
        {3, 2, 6, 6, 4, 1, 0, 0, 0, NULL},
        {4, 3, 4, 4, 1, 1, 0, 0, 0, NULL},
        {5, 4, 14, 14, 2, 1, 0, 0, 0, NULL}
    };

    // 준비 큐 선언 및 초기화
    Circular_Queue queue = {NULL, NULL};

    // 프로세스를 큐에 추가
    for (int i = 0; i < 5; i++) { 
        push_to_Q(&queue, processes[i]);
    }

    // 큐 내용 확인
    Process* temp = queue.head;
    if (temp != NULL) {
        do {
            temp = temp->next;
        } while (temp != queue.head);
    }

    pthread_t threads[5];

    // 스레드 생성 및 실행
    for (int i = 0; i < 5; i++) {
        sleep(1);
        pthread_create(&threads[i], NULL, process_thread, (void*)&queue);
    }

    // 스레드 종료 대기
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
    /* 간트 출력 */
    print_gant_chart(gant, 14);

    /* 각 프로세스의 반환시간과 대기시간 계산 */
    for (int i = 0; i < 5; i++) { // 프로세스 5개
        int process_id = processes[i].process_id;
        int last_end_time = -1;

        // 마지막 종료시간 찾기
        for (int j = 0; j < 14; j++) {
            if (gant[j].id == process_id && gant[j].end_time > last_end_time) {
                last_end_time = gant[j].end_time;
            }
        }

        // 반환시간 계산
        processes[i].turnAround_time = last_end_time - processes[i].arrival_time;

        printf("Process %d Turnaround Time: %d\n", process_id, processes[i].turnAround_time);
    
        // 대기시간 계산
        int last_start_time = -1;       // 최근 시작시간
        int total_execution_time = 0;   // 총 실행시간.

        for (int j = 0; j < 14; j++) {
            if (gant[j].id == process_id) {     // id가 같다면
                if (last_start_time == -1) {    // 처음 시작이다.
                    last_start_time = gant[j].start_time;
                }   
                total_execution_time += (gant[j].end_time - gant[j].start_time);
            }
        }

        processes[i].wait_time = processes[i].turnAround_time - total_execution_time;
        printf("Process %d Wait Time: %d\n", process_id, processes[i].wait_time);
    }

    // 평균 반환시간 및 평균 대기시간
    double average_turnAround_time = 0 ;
    double average_wait_time = 0 ; 
    
    for(int i = 0 ; i < 5; i++) { 
        average_turnAround_time += processes[i].turnAround_time;
        average_wait_time += processes[i].wait_time;
     }
    
    /* 평균 반환,대기 출력*/
    average_turnAround_time = average_turnAround_time/5.0;
    average_wait_time = average_wait_time/5.0;
    printf("average_turnAround_time: %lf\n average_wait_time: %lf\n", average_turnAround_time,average_wait_time);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}
