#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

typedef struct Node {
    int id;				// process id
    int runtime;		// 실행시간
    int arrivetime;		// 도착시간
    int starttime;		// 시작시간
    int endtime;		// 종료시간
    struct Node* next;	// 링크드리스트 구조
} Node;


/* 단순히 노드 만들기 */
Node* createNode(int id, int arrivetime, int runtime) {
    Node* newNode = malloc(sizeof(Node));
    
    newNode->id = id;
    newNode->arrivetime = arrivetime;
    newNode->runtime = runtime;
    newNode->starttime = 0;
    newNode->endtime = 0;
    newNode->next = NULL;
    return newNode;
}

void insertNode(Node** head, Node* newNode) {
    if (*head == NULL) {
		// 처음 노드라면,
        *head = newNode;
        newNode->starttime = newNode->arrivetime;
        newNode->endtime = newNode->starttime + newNode->runtime;
    }
    else {
		// 두번째부터,
		// 프로세스가 순서대로 들어오는 대로 처리하면 되기 때문에 바로 다음오는게 그 노드의 next이다.
        Node* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        newNode->starttime = current->endtime;  						// 이전 노드의 종료 시간이 새 노드의 시작 시간
        newNode->endtime = newNode->starttime + newNode->runtime;		// 종료시간
        current->next = newNode;										// 다음노드 연결
    }
}
/* Thread 실행함수 */
void* processNode(void* arg) {
    Node* node = (Node*)arg;
    for (int i = 1; i <= node->runtime; i++) {
        printf("P%d: %d * %d = %d\n", node->id, i, node->id, i * node->id);
    }
    return NULL;
}

int main() {
    Node* head = NULL;

	/* 단순히 노드 만들기*/
    Node* nodes[5];
    nodes[0] = createNode(1, 0, 10);
    nodes[1] = createNode(2, 1, 28);
    nodes[2] = createNode(3, 2, 6);
    nodes[3] = createNode(4, 3, 4);
    nodes[4] = createNode(5, 4, 14);
	
	/* 노드 삽입하기 */ 
    for (int i = 0; i < 5; i++) {
        insertNode(&head, nodes[i]);
    }
	
	Node* current = head;
    pthread_t threads[5];			// thread (=process) 5개

    int i = 0;
	// 처음부터 끝까지 순서대로 
    while (current != NULL) {
        pthread_create(&threads[i], NULL, processNode, (void*)current);	// 
        pthread_join(threads[i], NULL);  
        current = current->next;
        i++;
    }
	/* 간트차트 출력*/
	for(int i = 0 ;i<5;i++) { 
        printf("P%d (%d-%d)\n", nodes[i]->id, nodes[i]->starttime, nodes[i]->endtime);
	}
	
	double sum_turnaround_time = 0; 
	double sum_waiting_time = 0;

	/* 반환시간, 대기시간 출력 */ 
	for(int i =0 ; i< 5; i++) { 
		int turnAround_time = nodes[i]->endtime - nodes[i]->arrivetime;
		int waiting_time = nodes[i]->starttime - nodes[i]->arrivetime;
		
		sum_turnaround_time += turnAround_time; 
		sum_waiting_time += waiting_time;
	    printf("P%d: 반환시간 = %d, 대기시간 = %d\n",nodes[i]->id, turnAround_time, waiting_time);
	}

    printf("평균 반환시간 = %f\n", sum_turnaround_time / 5);
    printf("평균 대기시간 = %f\n", sum_waiting_time / 5);

    return 0;
}