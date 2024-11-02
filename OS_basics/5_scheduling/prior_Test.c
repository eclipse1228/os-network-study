#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

// Define the Process structure
typedef struct Process {
    int process_id;
    int arrival_time; // 도착시간
    int burst_time; // 실행 시간 (초)
    int remaining_time; // 남은 실행 시간
    int priority; // 우선순위
    int active; // 프로세스 활성 상태 
    struct Process *next; // 다음 프로세스
} Process;

// Define the Interval structure for Gantt chart
typedef struct Interval {
    int process_id;
    int start_time;
    int end_time;
    struct Interval *next;
} Interval;

// Define the LinkedList structure for both processes and intervals
typedef struct {
    Process *head;
    Process *tail;
} ProcessList;

typedef struct {
    Interval *head;
    Interval *tail;
} IntervalList;

// Define the Gant structure
typedef struct Gant {
    int process_id;
} Gant;

// Initialize the process list
void initProcessList(ProcessList *list) {
    list->head = list->tail = NULL;
}

// Initialize the interval list
void initIntervalList(IntervalList *list) {
    list->head = list->tail = NULL;
}

// Enqueue a process into the process list based on priority
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

// Remove a process from the process list
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

// Create a new interval
Interval* createInterval(int process_id, int start_time, int end_time) {
    Interval *newInterval = (Interval*)malloc(sizeof(Interval));
    newInterval->process_id = process_id;
    newInterval->start_time = start_time;
    newInterval->end_time = end_time;
    newInterval->next = NULL;
    return newInterval;
}

// Add an interval to the interval list
void addInterval(IntervalList *list, int process_id, int start_time, int end_time) {
    Interval *newInterval = createInterval(process_id, start_time, end_time);
    if (list->head == NULL) {
        list->head = list->tail = newInterval;
    } else {
        list->tail->next = newInterval;
        list->tail = newInterval;
    }
}

// Print the intervals
void printIntervals(IntervalList *list) {
    Interval *current = list->head;
    while (current != NULL) {
        printf("P%d (%d-%d)\n", current->process_id, current->start_time, current->end_time);
        current = current->next;
    }
}

// Print the process list
void printProcessList(ProcessList *list) {
    Process *current = list->head;
    while (current != NULL) {
        printf("P%d: Arrival Time = %d, Burst Time = %d, Remaining Time = %d, Priority = %d\n", 
                current->process_id, current->arrival_time, current->burst_time, current->remaining_time, current->priority);
        current = current->next;
    }
}
// 아무래도 작업함수가 잘 못 된 것 같다. 
// 
// 작업 함수
void working_func(int process_id, int start, int end, int *current_time, int *remaining_time) {
    for (int i = start; i < end; i++) {
        int work_done = (*current_time) - (*remaining_time);
        printf("P%d: %d * %d = %d\n", process_id, process_id, work_done + 1, process_id * (work_done + 1));
        (*current_time)++;
        (*remaining_time)--;
    }
}

int main() {
    // Define the processes
    Process processes[5] = {
        {1, 0, 10, 10, 3, 2, NULL},
        {2, 1, 28, 28, 2, 2, NULL},
        {3, 2, 6, 6, 4, 2, NULL},
        {4, 3, 4, 4, 1, 2, NULL},
        {5, 4, 14, 14, 2, 2, NULL}
    };

    // Initialize the ready queue
    ProcessList readyQueue;
    initProcessList(&readyQueue);

    // Enqueue processes to the ready queue
    for (int i = 0; i < 5; i++) {
        enqueue(&readyQueue, &processes[i]);
    }

    // Print the process list
    printf("Initial Process List:\n");
    printProcessList(&readyQueue);

    // Define the Gantt chart array
    Gant gant[62];
    int time = 0;
    Process *active_pkt = NULL;

    // Generate the Gantt chart
    for (int i = 0; i < 62; i++) {
        Process *p = readyQueue.head;
        Process *arrival_pkt = NULL;

        // Find the highest priority process that arrives at the current time
        while (p != NULL) {
            if (time >= p->arrival_time && p->remaining_time > 0) {
                if (!arrival_pkt || p->priority > arrival_pkt->priority) {
                    arrival_pkt = p;
                }
                // If priorities are the same, select the one with the earlier arrival time
                if (p->priority == arrival_pkt->priority) {
                    if (p->arrival_time < arrival_pkt->arrival_time) {
                        arrival_pkt = p;
                    }
                }
            }
            p = p->next;
        }

        // Replace active process if the new arrival has higher priority
        if (arrival_pkt && (!active_pkt || arrival_pkt->priority > active_pkt->priority)) {
            active_pkt = arrival_pkt;
        }

        // If no new arrival, find the highest priority process
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

        // Record to the Gantt chart
        if (active_pkt && active_pkt->remaining_time > 0) {
            gant[i].process_id = active_pkt->process_id;
            active_pkt->remaining_time--;

            // Remove process from the ready queue if its remaining time is 0
            if (active_pkt->remaining_time == 0) {
                removeProcess(&readyQueue, active_pkt->process_id);
                active_pkt = NULL;
            }
        } else {
            gant[i].process_id = 0; // No process running
        }

        time++;
    }

    // Use the Gantt chart to create the interval list
    IntervalList working_Qeueu;
    initIntervalList(&working_Qeueu);

    int current_process = gant[0].process_id;
    int start_time = 0;

    for (int i = 1; i < 62; i++) {
        if (gant[i].process_id != current_process) {
            addInterval(&working_Qeueu, current_process, start_time, i);
            current_process = gant[i].process_id;
            start_time = i;
        }
    }
    // Add the last interval
    addInterval(&working_Qeueu, current_process, start_time, 62);

    // Print the intervals
    printIntervals(&working_Qeueu);

    // 멀티 프로세스로 실제 계산하기
    Interval *current = working_Qeueu.head;
    while (current != NULL) {
        pid_t pid = fork();
        sleep(1);
        if (pid == 0) {
            for (int i = 0; i < 5; i++) {
                if (processes[i].process_id == current->process_id) {
                    int remaining_time = processes[i].remaining_time;
                    working_func(current->process_id, current->start_time, current->end_time, &(processes[i].burst_time), &remaining_time);
                    processes[i].remaining_time = remaining_time;
                    break;
                }
            }
            exit(0);
        }
        current = current->next;
    }

    while (wait(NULL) > 0); // Wait for all child processes to finish

    // Calculate turnaround time and waiting time
    int total_turnaround_time = 0;
    int total_waiting_time = 0;

    for (int i = 0; i < 5; i++) {
        int finish_time = 0;
        int start_time = 62;
        current = working_Qeueu.head;
        while (current != NULL) {
            if (current->process_id == processes[i].process_id) {
                if (current->end_time > finish_time) {
                    finish_time = current->end_time;
                }
                if (current->start_time < start_time) {
                    start_time = current->start_time;
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

    printf("Average Turnaround Time = %.2f\n", (float)total_turnaround_time / 5);
    printf("Average Waiting Time = %.2f\n", (float)total_waiting_time / 5);

    return 0;
}
