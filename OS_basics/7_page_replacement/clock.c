#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

// 노드 구조체 정의
typedef struct node {
    int value;         // 노드의 값을 저장하는 변수 (페이지 번호)
    int ref_bit;       // 참조 비트를 저장하는 변수 (1: 최근에 참조됨, 0: 참조되지 않음)
    struct node *next; // 다음 노드를 가리키는 포인터 (원형 큐 구조)
} node;

// 원형 큐 구조체 정의
typedef struct Circular_queue {
    node *head; // 원형 큐의 첫 번째 노드를 가리키는 포인터
    node *tail; // 원형 큐의 마지막 노드를 가리키는 포인터
    int size;   // 원형 큐의 크기 (현재 저장된 노드 수)
} cq;

// 원형 큐 초기화 함수
void init_cq(cq *q) {
    q->head = NULL;  // 원형 큐의 head를 NULL로 초기화
    q->tail = NULL;  // 원형 큐의 tail을 NULL로 초기화
    q->size = 0;     // 원형 큐의 크기를 0으로 초기화
}

// 새로운 노드를 생성하는 함수
node* create_node(int value) {
    node *new_node = (node*)malloc(sizeof(node)); // 노드 메모리 할당
    new_node->value = value;  // 노드 값 설정
    new_node->ref_bit = 1;    // 참조 비트 초기값 설정 (1)
    new_node->next = NULL;    // 다음 노드 포인터 초기화
    return new_node;          // 새로운 노드 반환
}

// 원형 큐에 노드를 추가하는 함수
void enqueue(cq *q, int value) {
    node *new_node = create_node(value); // 새로운 노드 생성
    if (q->size == 0) {  // 큐가 비어있는 경우
        q->head = new_node;        // head와 tail이 모두 새로운 노드를 가리킴
        q->tail = new_node;
        new_node->next = new_node; // 자신을 가리키도록 설정 (원형 구조)
    } else {  // 큐가 비어있지 않은 경우
        q->tail->next = new_node;  // tail의 다음 노드로 새로운 노드 설정
        new_node->next = q->head;  // 새로운 노드의 다음 노드를 head로 설정
        q->tail = new_node;        // tail을 새로운 노드로 업데이트
    }

    // new_node를 제외한 나머지 노드의 ref_bit를 0으로 설정
    node *current = q->head;
    do {
        if (current != new_node) {
            current->ref_bit = 0;
        }
        current = current->next;
    } while (current != q->head);

    q->size++; // 큐 크기 증가
}

// 현재 값이 큐에 있는지 확인하고, 있으면 참조 비트를 업데이트하는 함수
int page_fault_check(cq *q, int value) {
    node *current = q->head;
    for (int i = 0; i < q->size; i++) {
        if (current->value == value) {  // 현재 값이 큐에 있는 경우
            current->ref_bit = 1;       // 참조 비트를 1로 설정
            // 현재 노드를 제외한 나머지 노드들의 ref_bit를 0으로 설정
            node *temp = q->head;
            do {
                if (temp != current) {
                    temp->ref_bit = 0;
                }
                temp = temp->next;
            } while (temp != q->head);
            return 0;  // 페이지 폴트 아님
        }
        current = current->next; // 다음 노드로 이동
    }
    return 1;  // 페이지 폴트
}

// 페이지를 교체하는 함수 (Clock 알고리즘 구현)
void replace(cq *q, int value) {
    node *current = q->head;
    node *prev = NULL;

    // ref_bit이 1인 노드를 찾아 해당 다음 노드에 value 할당
    do {
        if (current->ref_bit == 1) {
            current= current->next;
            current->value = value;
            current->ref_bit = 1;
            // 나 말고 다른 것들은 bit =0;
            node *temp = q->head;
            do {
                if (temp != current) {
                    temp->ref_bit = 0;
                }
                temp = temp->next;
            } while (temp != q->head);
                    break;
        }
        prev = current;
        current = current->next;
    } while (current != q->head);

    // 해당 노드를 제외한 나머지 노드의 ref_bit을 0으로 설정

}
// 현재 큐 상태를 출력 배열에 저장하는 함수
void print_frames(cq *q, int time, int result_ref_array[3][20], int result_ref_bit_array[3][20]) {
    node *current = q->head;
    for (int i = 0; i < q->size; i++) { // 큐의 모든 노드를 순회
        result_ref_array[i][time] = current->value; // 현재 노드의 값을 저장
        result_ref_bit_array[i][time] = current->ref_bit; // 현재 노드의 참조 비트를 저장
        current = current->next; // 다음 노드로 이동
    }
    for (int i = q->size; i < 3; i++) { // 큐의 크기보다 작은 경우
        result_ref_array[i][time] = 0; // 나머지 공간은 0으로 채움
        result_ref_bit_array[i][time] = 0; // 나머지 공간의 참조 비트도 0으로 채움
    }
}
int main() {
    int ref_array[20] = {1, 2, 3, 2, 1, 5, 2, 1, 6, 2, 5, 6, 3, 1, 3, 6, 1, 2, 4, 3}; // 참조 문자열 배열
    int result_ref_array[3][20] = {0};   // 각 시간에 각 프레임의 값을 저장할 배열
    int result_ref_bit_array[3][20] = {0}; // 각 시간에 각 프레임의 참조 비트를 저장할 배열
    int frame_size = 3;                  // 프레임 크기
    int page_fault_count = 0;            // 페이지 폴트 카운트 초기화
    int page_fault_indices[20];          // 페이지 폴트 발생 인덱스를 저장할 배열
    int page_fault_index = 0;            // 페이지 폴트 발생 인덱스 카운터

    cq q;  // 원형 큐 선언
    init_cq(&q); // 원형 큐 초기화

    for (int i = 0; i < 20; i++) { // 참조 문자열 배열을 순회
        int current_value = ref_array[i]; // 현재 값을 가져옴
        if (page_fault_check(&q, current_value)) { // 페이지 폴트 발생 여부 확인
            page_fault_indices[page_fault_index++] = i; // 페이지 폴트 발생 인덱스 저장
            if (q.size == frame_size) { // 큐가 가득 찬 경우
                replace(&q, current_value); // 페이지 교체
            } else {                    // 큐 아직 덜 찼다.
                enqueue(&q, current_value); // 큐에 현재 값 추가
            }
        }
        else { // 페이지 폴트 발생 X
            node *current = q.head;
            while (current->value != current_value) {
                current = current->next;
            }
            current->ref_bit = 1;
            // 현재 노드를 제외한 나머지 노드들의 ref_bit를 0으로 설정
            node *temp = q.head;
            do {
                if (temp != current) {
                    temp->ref_bit = 0;
                }
                temp = temp->next;
            } while (temp != q.head);
        }
        print_frames(&q, i, result_ref_array, result_ref_bit_array); // 현재 프레임 상태 저장
    }

    printf("Frame status Output 1 (value, ref_bit):\n");
    for (int i = 0; i < 20; i++) { // 시간 순서대로 프레임 상태 출력
        for (int j = 0; j < 3; j++) {
            printf("(%d, %d) ", result_ref_array[j][i], result_ref_bit_array[j][i]);
        }
        printf("\n");
    }

    printf("Output 2:\n");
    for (int i = 0; i < page_fault_index; i++) { // 페이지 폴트 발생 인덱스 출력
        printf("%d ", page_fault_indices[i]);
    }
    printf("\n");

    return 0;
}