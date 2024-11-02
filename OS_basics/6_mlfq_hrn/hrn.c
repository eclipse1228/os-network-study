#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#define SUM_OF_BURSTTIME 62 // 프로세스 총 실행시간의 합 

/* 프로세스 구조체 */
typedef struct Process {
    int process_id;          // 프로세스 이름
    int burst_time;          // 실행시간
    int arrival_time;        // 도착시간
    int remaining_time;      // 남은 실행시간 (선점을 위한 비교 대상)
    struct Process *next;    // ReadyQueue는 LinkedList로 구현 (Next thread)
} Process;

/* 간트 차트 구조체 */
typedef struct {
    int process_num;  // 프로세스 번호
    int start_time;   // 시작시간
    int end_time;     // 종료시간
} GantObj;

int current_time = 0;           // 현재 시간 (간트차트의 시간이기도 하다.)
int active_process = 0;         // 활성화된 프로세스 번호 (1~5까지 pid (=tid))
pthread_mutex_t mutex;          // pthread 라이브러리의 mutexlock 동기화 방식으로 쓰레드간 동기화를 진행한다.
GantObj gantt_chart[20];        // 간트차트 배열 
int gantt_size = 0;             // 간트차트의 사이즈 
int switching_available = 0;    // 프로세스가 진행중인지, 끝났는지에 따라 스위칭 가능한지 여부
Process *head = NULL;           // linkedlist의 처음 head는 NULL을 가르킨다.

/* 간트 차트에 간트 추가하기 */
void add_gantt_entry(int thread_id, int start, int end) {
    if (start != end) {
        gantt_chart[gantt_size++] = (GantObj){thread_id, start, end};
    }
}

/* thread 를 linkedList readyQueue에 추가하는 함수 */
void add_thread(Process *new_thread) {
    if (head == NULL) {
        new_thread->next = head;
        head = new_thread;
    } 
    /* 처음으로 들어오는 경우가 아니면 */    
    else {
        Process *current = head;
        /* LinkedList가 끝나지 않았고(= head다음이 NULL이 아님), 새로 들어온 쓰레드가 도착시간이 더 크다면 연결한다.*/
        while (current->next != NULL && current->next->arrival_time <= new_thread->arrival_time) {
            current = current->next;
        }
        new_thread->next = current->next;
        current->next = new_thread;
    }
}

/* Thread가 생성될 때 실행되는 함수 (= pthread_create()의 이벤트 함수) */
void* thread_function(void* arg) {
    Process* proc = (Process*)arg;
    int started = 0;
    
    /* 프로세스의 남은실행시간이 다 소진될 때 까지 */
    while (proc->remaining_time > 0) {
        pthread_mutex_lock(&mutex);
        /*  내가 활성화된 (=작업 가능한) 프로세스라면, */
        if (active_process == proc->process_id) {
            int thread_startTime = current_time;
            if (!started) {
                started = 1;
            }
            // 작업 가능하면 작업 실행시간만큼 다 해야함
            while (proc->remaining_time > 0) {
                printf("P%d: %d * %d = %d\n", proc->process_id, proc->burst_time + 1 - proc->remaining_time, proc->process_id, (proc->burst_time + 1 - proc->remaining_time) * proc->process_id);
                proc->remaining_time--;
                current_time++;
            }
            /* 다 실행했으니, 기록*/
            add_gantt_entry(proc->process_id, thread_startTime, current_time);
            active_process = 0;
            started = 0;
            pthread_mutex_unlock(&mutex);
            usleep(100000);
        } 
        /*  내가 비활성화된 프로세스라면, (내 turn이 아니다.)*/
        else {
            pthread_mutex_unlock(&mutex);
            usleep(50000);
        }
    }
    return NULL;
}

/* HRN 스케줄링 함수 (return 프로세스 번호)*/
int priority_HRN(Process thread_array[], int current_time) {
    double max_prior = -1.0;    // 우선 순위
    int max_idx = -1;           // (프로세스 번호)
    for (int i = 0; i < 5; i++) {
        /*프로세스가 남은 시간이있고, 도착한 프로세스이면,*/
        if (thread_array[i].remaining_time > 0 && thread_array[i].arrival_time <= current_time) {
            double waiting_time = current_time - thread_array[i].arrival_time;
            double th_prior = (waiting_time + thread_array[i].burst_time) / thread_array[i].burst_time;
            if (th_prior > max_prior) {                 
                // 우선순위 비교, 프로세스번호 적용
                max_prior = th_prior;
                max_idx = i;
            }
        }
    }
    return max_idx;
}

int main() {
    pthread_t threads[5];
    Process thread_array[5] = {
        {1, 10, 0, 10, NULL}, {2, 28, 1, 28,  NULL}, {3, 6, 2, 6,  NULL},
        {4, 4, 3, 4, NULL}, {5, 14, 4, 14,  NULL}
    };
    // mutex를 사용하기 위해 초기화
    pthread_mutex_init(&mutex, NULL);

    // LinkedList에 삽입하고, pthread 5개 생성
    for (int i = 0; i < 5; i++) {
        add_thread(&thread_array[i]);
        pthread_create(&threads[i], NULL, thread_function, &thread_array[i]);
    }

    while (current_time <= SUM_OF_BURSTTIME - 1) {
        pthread_mutex_lock(&mutex);
        int hrn_idx = priority_HRN(thread_array, current_time); // 우선순위된 프로세스 번호
        // printf("hrn_idx = %d\n",hrn_idx);
        active_process = thread_array[hrn_idx].process_id;      // 활성 프로세스 
        pthread_mutex_unlock(&mutex);   // mutex 해제 
        usleep(100000); 
    }

    printf("\n");
    for (int i = 0; i < gantt_size; i++) {
        printf("P%d (%d-%d)\n", gantt_chart[i].process_num, gantt_chart[i].start_time, gantt_chart[i].end_time);
    }

    double avgTurn = 0, avgWait = 0;    // 평균반환시간, 평균 대기 시간
    int max_end_times[5] = {0};         // 반환시간을 위한 종료 시간

    // 간트에서 최대 종료시간 찾기 (반환시간을 계산하기 위해서)
    for (int i = 0; i < gantt_size; i++) {
        int thread_idx = gantt_chart[i].process_num - 1;
        if (max_end_times[thread_idx] < gantt_chart[i].end_time) {
            max_end_times[thread_idx] = gantt_chart[i].end_time;
        }
    }
    
    // 반환시간과 대기시간 계산하기
    for (int i = 0; i < 5; i++) {
        int turnaround_time = max_end_times[i] - thread_array[i].arrival_time;
        int waiting_time = turnaround_time - thread_array[i].burst_time;
        avgTurn += turnaround_time;
        avgWait += waiting_time;
        printf("P%d: 반환시간 = %d, 대기시간 = %d\n", thread_array[i].process_id, turnaround_time, waiting_time);
    }

    printf("평균 반환시간 = %f\n", avgTurn / 5);
    printf("평균 대기시간 = %f\n", avgWait / 5);

    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    return 0;
}

/*
비선점 sjf 의 비선점을 HRN 비선점 함수 하나로 수정해서 붙여넣었습니다. 멀티 쓰레딩 코드입니다.
조건은 (현재 시간보다 빨리 도착했고, 우선순위가 제일 많다. 그것이 active_process이다.)
대기시간은 현재시간(currnet_time) - 도착시간 (arrival_time) 으로 계산했습니다.
*/