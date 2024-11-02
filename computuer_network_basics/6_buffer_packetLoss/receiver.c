#include "Common.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>
#include <string.h>
#include <stdbool.h>

#define SERVERPORT 9000
#define PACKET_SIZE 7
#define BUFFER_SIZE 50

char *SERVER_IP = (char *)"127.0.0.1";

// 패킷을 저장할 메인 버퍼
unsigned char main_buffer[BUFFER_SIZE];
// 메인 버퍼에서 빈 공간의 시작 인덱스
int buffer_empty_index = 0;
// 스레드와 메인 함수 사이의 동기화 변수
int is_working = 0;
// 마지막으로 수신된 패킷 번호
int last_received_packet_num = -1;
// 마지막 패킷 번호
const int LAST_PACKET_NUM = 32;
// 송신자와의 통신을 위한 소켓
SOCKET socket_fd;

// wchar_t를 멀티바이트 문자로 변환하는 함수
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

// 체크섬을 계산하는 함수
void calculate_checksum(const wchar_t* wcs, bool* checksum) {
    int len = wcslen(wcs);

    memset(checksum, 0, 16 * sizeof(bool));

    for (int i = 0; i < len; i++) {
        bool binary[16] = {0};
        for (int j = 0; j < 16; j++) {
            binary[j] = (wcs[i] & (1 << (15 - j))) != 0;
        }

        bool carry = false;
        for (int j = 15; j >= 0; j--) {
            bool sum = checksum[j] ^ binary[j] ^ carry;
            carry = (checksum[j] && binary[j]) || (carry && (checksum[j] ^ binary[j]));
            checksum[j] = sum;
        }

        if (carry) {
            for (int j = 15; j >= 0; j--) {
                bool sum = checksum[j] ^ carry;
                carry = checksum[j] && carry;
                checksum[j] = sum;
            }
        }
    }
}

// 두 체크섬을 비교하는 함수
int compare_checksums(bool* checksum1, bool* checksum2) {
    for (int i = 0; i < 16; i++) {
        if (checksum1[i] != checksum2[i]) return 0;
    }
    return 1;
}

// 패킷 수신을 담당하는 스레드 함수
void* packet_receiver(void* arg) {
    int ret;
    unsigned char recv_buffer[PACKET_SIZE];

    while (last_received_packet_num < LAST_PACKET_NUM) {
        ret = recv(socket_fd, recv_buffer, PACKET_SIZE, 0);

        if (buffer_empty_index + PACKET_SIZE < BUFFER_SIZE) {
            memcpy(main_buffer + buffer_empty_index, recv_buffer, PACKET_SIZE);
            buffer_empty_index += PACKET_SIZE;
        }

        is_working = 0;
    }

    pthread_exit(NULL);
}

// 소켓 초기화 함수
void initialize_socket(const char *server_ip) {
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd == INVALID_SOCKET) err_quit("socket()");

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, server_ip, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(SERVERPORT);

    if (connect(socket_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR) {
        err_quit("connect()");
    }
}

// 수신된 패킷을 처리하는 함수
void process_packets(FILE *file) {
    unsigned char ack_buffer;
    char checksum_status[50];
    char ack_status[15];
    unsigned char packet[PACKET_SIZE];

    while (last_received_packet_num < LAST_PACKET_NUM) {
        usleep(100000);

        while (is_working == 1) {}

        memcpy(packet, main_buffer, PACKET_SIZE);
        memmove(main_buffer, main_buffer + PACKET_SIZE, BUFFER_SIZE - PACKET_SIZE);
        buffer_empty_index -= PACKET_SIZE;

        is_working = 1;

        unsigned char packet_num = packet[6] / 7;

        if (packet_num == (unsigned char)(last_received_packet_num + 1)) {
            last_received_packet_num++;
        }

        sprintf(ack_status, "ACK = %d", last_received_packet_num * 7);
        ack_buffer = (unsigned char)last_received_packet_num * 7;

        wchar_t payload[2];
        for (int i = 0; i < 2; i++) {
            payload[i] = (wchar_t)((packet[i * 2] << 8) | packet[i * 2 + 1]);
        }

        bool received_checksum[16];
        int high_byte = packet[4];
        for (int i = 7; i >= 0; i--) {
            received_checksum[i] = high_byte % 2;
            high_byte /= 2;
        }

        int low_byte = packet[5];
        for (int i = 15; i >= 8; i--) {
            received_checksum[i] = low_byte % 2;
            low_byte /= 2;
        }

        bool calculated_checksum[16];
        calculate_checksum(payload, calculated_checksum);

        if (compare_checksums(calculated_checksum, received_checksum)) {
            sprintf(checksum_status, "and there is no error.");
        } else {
            sprintf(checksum_status, "and there is error!");
        }

        char payload_str[200];
        int len = 0;
        for (int i = 0; i < 2; i++) {
            len += wchar_to_multibyte(payload[i], payload_str + len, sizeof(payload_str));
        }
        payload_str[len] = '\0';

        char network_text[256];
        sprintf(network_text, "packet %d is received %s (%s) (%s) is transmitted.\n",
                packet_num, checksum_status, payload_str, ack_status);
        printf("%s", network_text);
        fputs(network_text, file);

        send(socket_fd, &ack_buffer, 1, 0);
    }
}

// 메인 함수
int main(int argc, char *argv[]) {
    setlocale(LC_ALL, "");

    if (argc > 1) SERVER_IP = argv[1];

    initialize_socket(SERVER_IP);

    FILE *file = fopen("output.txt", "w");
    fputs("---receiver---\n", file);

    pthread_t receiver_thread;
    pthread_create(&receiver_thread, NULL, packet_receiver, NULL);

    process_packets(file);

    fclose(file);
    close(socket_fd);

    return 0;
}
