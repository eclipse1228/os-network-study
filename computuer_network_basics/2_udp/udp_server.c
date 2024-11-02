#include "Common.h"

#define SERVERPORT 9000 // 서버 포트 9000번 사용 
#define BUFSIZE    512  // 버퍼 사이즈 

int main(int argc, char *argv[])
{
	int retval; // 연결 성공 실패를 0,1로 나타냄

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;      // 소켓을 주소와 연결하기
	memset(&serveraddr, 0, sizeof(serveraddr));     // 서버주소를 0으로 전부 초기화 (연결에 문제 생기는것 방지)
	serveraddr.sin_family = AF_INET; // 인터넷 주소 체계
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); // 즉 IP 주소체계를 빅엔디안 방식으로 바꾸라는것 , host to network Long (IP는 4바이트 long)
	serveraddr.sin_port = htons(SERVERPORT); // 포트 주소체계를 리틀엔디안으로 바꾸기
	retval = bind(sock, (struct sockaddr *)&serveraddr, sizeof(serveraddr)); // 소켓을 서버주소와 연결하기
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// 데이터 통신에 사용할 변수
	struct sockaddr_in clientaddr;  // 사용자 주소
	socklen_t addrlen;              // 길이
	char buf[BUFSIZE + 1];          // 버퍼 선언

	// 클라이언트와 데이터 통신
	while (1) {
		// 데이터 받기
		addrlen = sizeof(clientaddr); // 크기 
		retval = recvfrom(sock, buf, BUFSIZE, 0, 
			(struct sockaddr *)&clientaddr, &addrlen);
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			break;
		}

		// 받은 데이터 출력
		buf[retval] = '\0';
		char addr[INET_ADDRSTRLEN]; 
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr)); // ip주소를 사람이 볼 수 있게 한다.
		printf("[UDP/%s:%d] %s\n", addr, ntohs(clientaddr.sin_port), buf); // 접속 정보 출력

		// 데이터 보내기
		retval = sendto(sock, buf, retval, 0,
			(struct sockaddr *)&clientaddr, sizeof(clientaddr));
		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			break;
		}
	}

	// 소켓 닫기
	close(sock);
	return 0;
}