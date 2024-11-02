#include <stdio.h>
#include <stdbool.h>

#define FRAME_SIZE 3       // 프레임 크기
#define REF_ARRAY_SIZE 20  // 참조 배열 크기

// 참조 배열 초기화
int ref_array[REF_ARRAY_SIZE] = {1, 2, 3, 2, 1, 5, 2, 1, 6, 2, 5, 6, 3, 1, 3, 6, 1, 2, 4, 3}; 
int result[FRAME_SIZE][REF_ARRAY_SIZE] = {0}; // 결과 2차원 배열
int frame[FRAME_SIZE] = {0}; // 물리 메모리 (프레임)
int page_faults[REF_ARRAY_SIZE] = {0}; // 페이지 부재 배열
int stack[FRAME_SIZE]; // 스택

// 페이지가 프레임에 있는지 확인하는 함수
int find_page(int page) {
    for (int i = 0; i < FRAME_SIZE; i++) {
        if (frame[i] == page) {
            return i; // 페이지가 프레임에 있으면 해당 인덱스를 반환
        }
    }
    return -1; // 페이지가 프레임에 없으면 -1을 반환
}

// 페이지를 스택의 맨 위로 이동하는 함수
void move_to_top(int index) {
    int page = frame[index];
    // 인덱스 위치부터 스택의 맨 위까지 페이지를 이동
    for (int i = index; i > 0; i--) {
        frame[i] = frame[i - 1];
    }
    frame[0] = page; // 페이지를 스택의 맨 위로 이동
}

// LRU 페이지 교체 알고리즘 함수
void lru_page_replacement() {
    for (int i = 0; i < FRAME_SIZE; i++) {
        stack[i] = -1; // 스택을 -1로 초기화 (빈 상태)
    }

    for (int i = 0; i < REF_ARRAY_SIZE; i++) { // 참조 배열의 모든 페이지에 대해 반복
        int page = ref_array[i];
        int index = find_page(page); // 페이지가 프레임에 있는지 확인

        if (index != -1) {
            // 페이지가 프레임에 존재하는 경우
            move_to_top(index); // 해당 페이지를 스택의 맨 위로 이동
        } else {
            // 페이지 부재 발생
            page_faults[i] = 1; // 페이지 부재 기록

            // 프레임에 빈 공간이 있는지 확인
            if (find_page(0) != -1) {
                int empty_index = find_page(0);
                frame[empty_index] = page; // 빈 공간에 페이지를 삽입
                move_to_top(empty_index); // 삽입한 페이지를 스택의 맨 위로 이동
            } else {
                // 프레임이 꽉 찬 경우, 스택의 맨 아래 페이지를 교체
                for (int j = FRAME_SIZE - 1; j > 0; j--) {
                    frame[j] = frame[j - 1];
                }
                frame[0] = page; // 새로운 페이지를 스택의 맨 위에 삽입
            }
        }

        // 현재 프레임 상태를 결과 배열에 저장
        for (int j = 0; j < FRAME_SIZE; j++) {
            result[j][i] = frame[j];
        }
    }
}

int main() {
    lru_page_replacement(); // LRU 페이지 교체 알고리즘 실행

    // 출력 2: 페이지 부재가 발생한 참조 문자열의 인덱스 출력
    printf("출력 2:\n");
    for (int i = 0; i < REF_ARRAY_SIZE; i++) {
        if (page_faults[i]) {
            printf("%d ", i); // 페이지 부재가 발생한 인덱스를 출력
        }
    }
    printf("\n");

    // 출력 1: 각 시간에 각 프레임의 상태 출력
    printf("출력 1:\n");
    for (int i = 0; i < FRAME_SIZE; i++) {
        for (int j = 0; j < REF_ARRAY_SIZE; j++) {
            if (result[i][j] == -1) {
                printf("0 ");  // 초기화되지 않은 상태를 0으로 표시
            } else {
                printf("%d ", result[i][j]); // 현재 프레임의 상태를 출력
            }
        }
        printf("\n");
    }

    return 0;
}
