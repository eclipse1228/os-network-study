#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
/*
프로세스의 역할을 쓰레드가 대신 하고 있다.
큰 범위에서는 프로세스라고 변수명을 지었으며, pthread가 나오고 부터는 쓰레드라고 변수명을 지었다. 
선점을 먼저 구현했었다. 선점sjf에서 조건에서 선점하지 못 하게하면 구현된다.
*/
#define SUM_OF_BURSTTIME 62     // 프로세스 총 실행시간의 합 

/* 프로세스 구조체 */
typedef struct Process {
    int process_id;             // 프로세스 이름
    int burst_time;             // 실행시간
    int arrival_time;           // 도착시간
    int remaining_time;      // 남은 실행시간 (선점을 위한 비교 대상)
    struct Process *next;       // ReadyQueue는 LinkedList로 구현 (Next thread)
} Process;

/* 간트 차트 구조체 */
typedef struct {
    int process_num; // 프로세스 번호
    int start_time; // 시작시간
    int end_time;   // 종료시간
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
    /* thread가 Linkedlist 에 처음 들어오는 경우*/
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
        new_thread->next = current->next;      // link 연결
        current->next = new_thread;
    }
}
/* Thread가 생성될 때 실행되는 함수 (= pthread_create()의 이벤트 함수) */
void* thread_function(void* arg) {
    Process* proc = (Process*)arg;
    int started = 0;                                                // started 변수는 스레드가 실행을 시작했는지를 나타낸다. 동시에 gantchart에서 사용하기 위함이기도 하다.

    /* 프로세스의 남은실행시간이 다 소진될 때 까지 */
    while (proc->remaining_time > 0) {
        // printf("process %d waiting..",proc->process_id);
        pthread_mutex_lock(&mutex);                                 // mutex_lock으로 선점하기 (만약 최초의 lock이 아니라, 이미 선점된 상태에서 mutex_lock을 어딘가에서 사용하면 해당 프로세스는 대기상태가 됨)
        /*  내가 활성화된 (=작업 가능한) 프로세스라면, */
        if (active_process == proc->process_id) {         
            int thread_startTime = current_time;          
            if (!started) {                                         // 만약, 아직 시작하지 않았다면
                started = 1;                                        // 시작했다.
            }
            
            // 작업 가능하면 작업 실행시간만큼 다 해야함
            while(proc->remaining_time > 0 ){
                printf("P%d: %d * %d = %d\n", proc->process_id, proc->burst_time + 1 - proc->remaining_time, proc->process_id, (proc->burst_time + 1 - proc->remaining_time) * proc->process_id);  
                proc->remaining_time--;
                current_time++;
            }
            /* 다 실행했으니, 기록*/
                add_gantt_entry(proc->process_id, thread_startTime ,current_time);
                active_process = 0;
                started = 0;
            pthread_mutex_unlock(&mutex);                                               // lock해제 함으로써 다른 프로세스 실행됨
            usleep(100000);                                                             // 0.1초 sleep (하지 않으면, 동기화 되지 않음. main에서 active_process를 0.1초 타이밍으로 맞춰놓았기 때문이다.)                                
        }
        /*  내가 비활성화된 프로세스라면, (내 turn이 아니다.)*/
        else {                                                                          
            pthread_mutex_unlock(&mutex);                           // unlock 시키기
            usleep(50000);                                          // 0.05초 sleep 
        }
    }
    return NULL;
}

int main() {
    pthread_t threads[5];         
    Process thread_array[5] = {
        {1, 10, 0, 10, NULL}, {2, 28, 1, 28,  NULL}, {3, 6, 2, 6,  NULL},
        {4, 4, 3, 4, NULL}, {5, 14, 4, 14,  NULL}
    };

    pthread_mutex_init(&mutex, NULL);                                                       // mutex를 사용하기 위해 초기화

    for (int i = 0; i < 5; i++) {                                                           // LinkedList에 삽입하고, pthread 5개 생성
        add_thread(&thread_array[i]);
        pthread_create(&threads[i], NULL, thread_function, &thread_array[i]);
    }
    
    /* 총 실행시간 만큼 진행한다. */
    while (current_time <= SUM_OF_BURSTTIME - 1) {
        pthread_mutex_lock(&mutex);                                                         // mutex_lock 
        int min_burst = 9999;                                                           // 최소 남은 실행시간을 찾는다.
        Process *current = head;
        /* 활성 프로세스를 정한다. current(head)가 NULL될 때 까지 == 연결리스트 끝까지  */
        while (current) {
            // 현재 시간보다 빨리 도착했고, burst 시간이 제일 짧다. 그것이 active_process이다. 선점 sjf
            // 현재 시간보다 빨리 도착했고, burst 시간이 제일 짧다. 오로지 실행이 끝난 시점에서만 스위칭 가능하다. 비선점 sjf
            if(current->burst_time < min_burst && current->arrival_time <= current_time && current->burst_time < min_burst && current->remaining_time != 0 ) { 
                min_burst = current->burst_time;
            }
            current = current->next;
        }
        for(int i=0;i<5;i++) {
            // 최소시간인 프로세스, 이미 종료된것 제외해서 active. 
            if(thread_array[i].burst_time == min_burst && thread_array[i].remaining_time != 0){
                active_process = thread_array[i].process_id;
            }
        }
        pthread_mutex_unlock(&mutex);                                                      // unlock
        usleep(100000);                                                                    // 0.1 초 sleep
    }
    // 간트차트
    printf("\n");
    for (int i = 0; i < gantt_size; i++) {
        printf("P%d (%d-%d)\n", gantt_chart[i].process_num, gantt_chart[i].start_time, gantt_chart[i].end_time);
    }

    double avgTurn = 0, avgWait = 0;                                                        // 평균반환시간, 평균 대기 시간
    int max_end_times[5] = {0};                                                             // 반환시간을 위한 종료 시간

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

    pthread_mutex_destroy(&mutex);                                                          // mutex 종료
    return 0;
}
