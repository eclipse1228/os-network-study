#include <stdio.h>
#include <stdlib.h>

#define FRAME_SIZE 3       // 프레임 크기
#define REF_ARRAY_SIZE 20  // 참조 배열 크기

int ref_array[REF_ARRAY_SIZE] = {1, 2, 3, 2, 1, 5, 2, 1, 6, 2, 5, 6, 3, 1, 3, 6, 1, 2, 4, 3};  // 참조 배열
int frame[FRAME_SIZE] = {0};  // 프레임
int result[FRAME_SIZE][REF_ARRAY_SIZE] = {0};  // 결과 배열

// 페이지가 프레임에 있는지 확인하는 함수
int is_in_frame(int page) {
    for (int i = 0; i < FRAME_SIZE; i++) {
        if (frame[i] == page) {
            return 1;
        }
    }
    return 0;
}

// OPT 페이지 교체 알고리즘 함수
void opt_page_replacement() {
    int page_faults[REF_ARRAY_SIZE] = {0};  // 페이지 부재 배열
    int frame_age[FRAME_SIZE] = {0};  // 프레임 순서를 관리
    int front = 0, rear = 0;  // frame_age front와 rear 인덱스

    // 참조 배열을 순회하면서 페이지 교체 수행
    for (int i = 0; i < REF_ARRAY_SIZE; i++) {
        // 페이지 부재 발생
        if (!is_in_frame(ref_array[i])) {
            page_faults[i] = 1;

            // 프레임에 빈 공간이 있으면 페이지 추가
            if (rear - front < FRAME_SIZE) {
                frame[rear % FRAME_SIZE] = ref_array[i];
                frame_age[rear % FRAME_SIZE] = i;
                rear++;
            }
            // 프레임이 가득 차면 가장 멀리있는 페이지 교체
            else {
                int farthest = -1;
                int replace_idx = -1;

                // 프레임에서 가장 멀리있는 페이지 찾기
                for (int j = front; j < rear; j++) {
                    int k;
                    for (k = i + 1; k < REF_ARRAY_SIZE; k++) {
                        if (frame[j % FRAME_SIZE] == ref_array[k]) {
                            if (k > farthest) {
                                farthest = k;
                                replace_idx = j % FRAME_SIZE;
                            }
                            break;
                        }
                    }
                    if (k == REF_ARRAY_SIZE) {
                        replace_idx = (front + 1) % FRAME_SIZE;
                        break;
                    }
                }

                // 가장 멀리있는 페이지를 새로운 페이지로 교체
                frame[replace_idx] = ref_array[i];
                frame_age[replace_idx] = i;
                front = (front + 1) % FRAME_SIZE;
            }
        }

        // 결과 배열에 현재 프레임 상태 저장
        for (int j = 0; j < FRAME_SIZE; j++) {
            result[j][i] = frame[j];
        }
    }

    // 출력 2: 페이지 부재가 발생한 참조 문자열의 인덱스 출력
    printf("출력 2:\n");
    for (int i = 0; i < REF_ARRAY_SIZE; i++) {
        if (page_faults[i]) {
            printf("%d ", i);
        }
    }
    printf("\n");

    // 출력 1: 각 시간에 각 프레임의 상태 출력
    printf("출력 1:\n");
    for (int i = 0; i < FRAME_SIZE; i++) {
        for (int j = 0; j < REF_ARRAY_SIZE; j++) {
            printf("%d ", result[i][j]);
        }
        printf("\n");
    }
}

int main() {
    opt_page_replacement();
    return 0;
}