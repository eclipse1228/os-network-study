#include "Common.h"
#include <pthread.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#define SERVER_PORT 9000
#define TIMEOUT_SECONDS 0
#define TIMEOUT_MICROSECONDS 500000
#define PACKET_SIZE 7
#define BUFFER_SIZE 512
#define PAYLOAD_SIZE 2

// 통신 메시지를 저장할 배열

int ack_count[100]; // 각 패킷의 ACK 수신 횟수
SOCKET client_socket; // 클라이언트 소켓
int is_working = 0; // 작업 상태 플래그
char comm_messages[1000][100];
int ack_window_end = 3; // ACK 윈도우 끝 인덱스
unsigned char packet_buffer[100][PACKET_SIZE]; // 패킷 버퍼
char payload_strings[100][20]; // 페이로드 문자열
int last_comm_msg_index = 0; // 마지막 메시지 인덱스
int ack_window_start = 0; // ACK 윈도우 시작 인덱스
int final_packet_index = 0; // 마지막 패킷 인덱스

// 함수 선언
int wchar_to_multibyte(wchar_t wc, char *mb, size_t max_len);
void* ack_thread(void* arg);
void initialize_locale();
void read_text_file(const char* filename, wchar_t** wcs, size_t* len);
void generate_packets(const wchar_t* wcs);
void transmit_packets();
void write_comm_messages_to_file(const char* filename);

// 유니코드 문자를 멀티바이트 문자열로 변환하는 함수
int wchar_to_multibyte(wchar_t wc, char *mb, size_t max_len) {
    mbstate_t state;
    memset(&state, 0, sizeof(state));
    size_t len = wcrtomb(mb, wc, &state);
    if (len == (size_t)-1) {
        perror("wcrtomb");
        exit(1);
    }
    return len;
}

// ACK 수신을 처리하는 스레드 함수
void* ack_thread(void* arg) {
    unsigned char buf[2];
    int ret;
    printf("Thread started.\n");
    while (is_working == 0) {}
    while (ack_window_start < ack_window_end) {
        ret = recv(client_socket, buf, 1, 0);
        int packet_index = (int)buf[0] / PACKET_SIZE;
        buf[1] = '\0';
        
        if (ret == -1) {
            printf("\"packet %d\" is timeout.\n", ack_window_start);
            sprintf(comm_messages[last_comm_msg_index], "packet %d is timeout.\n", ack_window_start);
            last_comm_msg_index++;
        } else if (packet_index == ack_window_start) {
            ack_window_start++;
            if (ack_window_start > final_packet_index) {
                ack_window_start = final_packet_index;
            }
            ack_window_end++;
            if (ack_window_end > final_packet_index) {
                ack_window_end = final_packet_index;
            }
            printf("ack \"%d\" is received. \"packet %d\" is transmitted. (\"%s\")\n", buf[0], ack_window_end, payload_strings[ack_window_end]);
            sprintf(comm_messages[last_comm_msg_index], "(ACK = %d) is received and packet %d is transmitted. (\"%s\")\n",
                    buf[0], ack_window_end, payload_strings[ack_window_end]);
            last_comm_msg_index++;
            send(client_socket, packet_buffer[ack_window_end], PACKET_SIZE, 0);
            usleep(50000);
        } else {
            if (ack_count[packet_index] == 1) {
                printf("(ACK = %d) is received and ignored.\n", buf[0]);
                sprintf(comm_messages[last_comm_msg_index], "(ACK = %d) is received and ignored.\n", buf[0]);
                last_comm_msg_index++;
            }
            ack_count[packet_index]++;
        }
    }
    pthread_exit(NULL);
}

// 로케일 초기화 함수 ()
void initialize_locale() {
    if (setlocale(LC_ALL, "") == NULL) {
        perror("setlocale");
        exit(1);
    }
}

// 텍스트 파일을 읽어와 유니코드 문자열로 변환하는 함수
void read_text_file(const char* filename, wchar_t** wcs, size_t* len) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("fopen");
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    char *buffer = (char*)malloc(sizeof(char) * (file_size + 1));
    fread(buffer, 1, file_size, file);
    buffer[file_size - 1] = '\0';
    *len = mbstowcs(NULL, buffer, 0) + 1;
    *wcs = (wchar_t*)malloc(*len * sizeof(wchar_t));
    mbstowcs(*wcs, buffer, *len);
    fclose(file);
    free(buffer);
}

// 유니코드 문자열을 패킷으로 변환하는 함수
void generate_packets(const wchar_t* wcs) {
    int wcs_position = 0;
    while (wcs_position < wcslen(wcs)) {
        unsigned char packet_index = wcs_position / PAYLOAD_SIZE;
        final_packet_index = packet_index;
        unsigned char payload[PAYLOAD_SIZE][2];
        int len = 0;
        for (int j = 0; j < PAYLOAD_SIZE; j++) {
            len += wchar_to_multibyte(wcs[wcs_position + j], payload_strings[packet_index] + len, sizeof(payload_strings[packet_index]));
            int unicode_value = (int)wcs[wcs_position + j];
            payload[j][0] = (unicode_value & 0xFF00) >> 8;
            payload[j][1] = (unicode_value & 0x00FF);
        }
        payload_strings[packet_index][len] = '\0';
        bool checksum[16] = { false };
        bool binary[16] = { false };
        for (int j = 0; j < PAYLOAD_SIZE; j++) {
            for (int k = 0; k < 16; k++) {
                binary[k] = ((int)wcs[wcs_position + j] & (1 << (15 - k))) != 0;
            }
            bool carry = false;
            for (int k = 15; k >= 0; k--) {
                bool sum = checksum[k] ^ binary[k] ^ carry;
                carry = (checksum[k] && binary[k]) || (carry && (checksum[k] ^ binary[k]));
                checksum[k] = sum;
            }
            if (carry) {
                for (int k = 15; k >= 0; k--) {
                    bool sum = checksum[k] ^ carry;
                    carry = checksum[k] && carry;
                    checksum[k] = sum;
                }
            }
        }
        for (int j = 0; j < PAYLOAD_SIZE; j++) {
            for (int k = 0; k < 2; k++) {
                packet_buffer[packet_index][2 * j + k] = payload[j][k];
            }
        }
        unsigned char high_byte = 0, low_byte = 0;
        for (int j = 0; j < 7; j++) {
            high_byte = (high_byte << 1) | checksum[j];
        }
        packet_buffer[packet_index][4] = high_byte;
        for (int j = 8; j < 15; j++) {
            low_byte = (low_byte << 1) | checksum[j];
        }
        packet_buffer[packet_index][5] = low_byte;
        packet_buffer[packet_index][6] = packet_index * PACKET_SIZE;
        wcs_position += PAYLOAD_SIZE;
    }
}

// 패킷을 전송하는 함수
void transmit_packets() {
    pthread_t tid;
    pthread_create(&tid, NULL, ack_thread, NULL);
    while (ack_window_start < ack_window_end) {
        for (int i = ack_window_start; i <= ack_window_end; i++) {
            usleep(50000);
            printf("packet %d is transmitted. (\"%s\")\n", i, payload_strings[i]);
            sprintf(comm_messages[last_comm_msg_index], "packet %d is transmitted. (\"%s\")\n", i, payload_strings[i]);
            last_comm_msg_index++;
            send(client_socket, packet_buffer[i], PACKET_SIZE, 0);
        }
        is_working = 1;
    }
    pthread_join(tid, NULL);
}

// 통신 메시지를 파일에 저장하는 함수
void write_comm_messages_to_file(const char* filename) {
    FILE *file = fopen(filename, "a");
    if (!file) {
        perror("fopen");
        exit(1);
    }
    fputs("---sender---\n", file);
    for (int i = 0; i < last_comm_msg_index; i++) {
        fputs(comm_messages[i], file);
    }
    fclose(file);
}

int main(int argc, char *argv[]) {
    SOCKET listen_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len;
    char client_address[INET_ADDRSTRLEN];
    wchar_t *wide_chars;
    size_t text_length;

    // 소켓 생성
    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket == INVALID_SOCKET) err_quit("socket()");

    // 서버 주소 설정
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(SERVER_PORT);
    if (bind(listen_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) err_quit("bind()");

    // 연결 대기
    if (listen(listen_socket, SOMAXCONN) == SOCKET_ERROR) err_quit("listen()");

    // 클라이언트 연결 수락
    addr_len = sizeof(client_addr);
    client_socket = accept(listen_socket, (struct sockaddr *)&client_addr, &addr_len);
    struct timeval timeout = { TIMEOUT_SECONDS, TIMEOUT_MICROSECONDS };
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    inet_ntop(AF_INET, &client_addr.sin_addr, client_address, sizeof(client_address));

    // 로케일 초기화 및 파일 읽기
    initialize_locale();
    read_text_file("text.txt", &wide_chars, &text_length);
    generate_packets(wide_chars);
    free(wide_chars);
    transmit_packets();
    write_comm_messages_to_file("output.txt");
    
    // 소켓 닫기
    close(client_socket);
    close(listen_socket);

    return 0;
}
