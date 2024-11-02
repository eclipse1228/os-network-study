#include <stdio.h>
#include <stdlib.h>

typedef struct node {
    int value;
    struct node *next;
} node;

typedef struct queue {
    node *head;
    node *tail;
    int size;
} queue;

// FIFO 큐 초기화 함수
void init_queue(queue *q) {
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;
}

// 큐에 값을 추가하는 함수
void enqueue(queue *q, int value) {
    node *new_node = (node*)malloc(sizeof(node));
    new_node->value = value;
    new_node->next = NULL;
    if (q->tail != NULL) {
        q->tail->next = new_node;
    }
    q->tail = new_node;
    if (q->head == NULL) {
        q->head = new_node;
    }
    q->size++;
}

// 큐에서 값을 제거하는 함수
int dequeue(queue *q) {
    if (q->head == NULL) {
        return -1;
    }
    int value = q->head->value;
    node *temp = q->head;
    q->head = q->head->next;
    if (q->head == NULL) {
        q->tail = NULL;
    }
    free(temp);
    q->size--;
    return value;
}

// 큐에 특정 값이 존재하는지 확인하는 함수
int is_in_queue(queue *q, int value) {
    node *current = q->head;
    while (current != NULL) {
        if (current->value == value) {
            return 1;
        }
        current = current->next;
    }
    return 0;
}

// FIFO 페이지 교체 알고리즘 구현
int main() {
    queue ff_q;
    init_queue(&ff_q);

    int ref_array[20] = {1, 2, 3, 2, 1, 5, 2, 1, 6, 2, 5, 6, 3, 1, 3, 6, 1, 2, 4, 3}; // 참조 배열
    int result_ref_array[3][20] = {0}; // 결과 2차원 배열
    int frame[3] = {0}; // 물리 메모리 (프레임)
    int frame_size = 3;
    int frame_index = 0;
    printf("출력 2:\n");
    for (int i = 0; i < 20; i++) {
        int value = ref_array[i];

        if (!is_in_queue(&ff_q, value)) { // 페이지 부재 발생
            printf("%d ", i); // 출력2: 페이지 부재 인덱스 출력

            if (ff_q.size == frame_size) { // 메모리가 가득 찬 경우
                int removed = dequeue(&ff_q); // 가장 오래된 페이지 제거
                // 메모리에서 제거
                for (int j = 0; j < frame_size; j++) {
                    if (frame[j] == removed) {
                        frame[j] = value;
                        break;
                    }
                }
            } else { // 메모리가 가득 차지 않은 경우
                frame[frame_index++] = value;
            }
            enqueue(&ff_q, value); // 새 페이지 삽입
        }

        // 출력1: 각 시간의 FIFO queue 상태 result_ref_array에 저장
        for (int j = 0; j < frame_size; j++) {
            if (j < ff_q.size) {
                result_ref_array[j][i] = frame[j];
            } else {
                result_ref_array[j][i] = 0;
            }
        }
    }

    printf("\n");
    printf("출력 1:\n");
    // 출력1: 각 시간의 FIFO queue 상태 출력
    for (int i = 0; i < frame_size; i++) {
        for (int j = 0; j < 20; j++) {
            printf("%d ", result_ref_array[i][j]);
        }
        printf("\n");
    }

    return 0;
}
