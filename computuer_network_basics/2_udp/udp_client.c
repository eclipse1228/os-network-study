#include "Common.h"

char *SERVERIP = (char *)"127.0.0.1";// 서버 IP 주소(로컬)
#define SERVERPORT 9000				 // 서버 포트 9000번 사용 
#define BUFSIZE    512			     // 버퍼 사이즈 

int main(int argc, char *argv[])
{
	int retval; 					 // 연결 성공 실패를 0,1로 나타냄

	// 명령행 인수가 있으면 IP 주소로 사용
	if (argc > 1) SERVERIP = argv[1];

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);  
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// 소켓 주소 구조체 초기화
	struct sockaddr_in serveraddr; 					// 소켓 주소 구조체 선언
	memset(&serveraddr, 0, sizeof(serveraddr));		// 서버주소를 0으로 전부 초기화 (연결에 문제 생기는것 방지)
	serveraddr.sin_family = AF_INET;				// 인터넷 주소 체계를 사용한다.
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr); // IP주소를 네트워크 주소체계로 바꾸기
	serveraddr.sin_port = htons(SERVERPORT);		// 포트는 보통 2바이트 short, host to network short, 네트워크는 빅엔디안

	// 데이터 통신에 사용할 변수
	struct sockaddr_in peeraddr;	// 보내온 주소
	socklen_t addrlen;				// 주소 길이
	char buf[BUFSIZE + 1]; 			// 버퍼 선언
	int len;						// 길이 

	// 서버와 데이터 통신
	while (1) {
		// 데이터 입력
		printf("\n[보낼 데이터] ");
		if (fgets(buf, BUFSIZE + 1, stdin) == NULL) // fgets로 입력받기, 두번쨰 인자는 최대받을 사이즈
			break;

		// '\n' 문자 제거 -> 엔터치니까.
		len = (int)strlen(buf);
		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';
		if (strlen(buf) == 0) 						// 그냥 엔터만 누르면 종료시키기.
			break;

		// 데이터 보내기 (tcp처럼 connect() 없음)
		retval = sendto(sock, buf, (int)strlen(buf), 0,			// 서버 주소와 서버 주소 사이즈를 입력해야한다.
			(struct sockaddr *)&serveraddr, sizeof(serveraddr));
		if (retval == SOCKET_ERROR) {
			err_display("sendto()");
			break;
		}
		printf("[UDP 클라이언트] %d바이트를 보냈습니다.\n", retval);

		// 데이터 받기
		addrlen = sizeof(peeraddr);				// 상대 주소 저장
		retval = recvfrom(sock, buf, BUFSIZE, 0, 	 
			(struct sockaddr *)&peeraddr, &addrlen);	// 상대 주소, 주소 사이즈 있어야함 
		if (retval == SOCKET_ERROR) {
			err_display("recvfrom()");
			break;
		}

		// 송신자의 주소 체크
		if (memcmp(&peeraddr, &serveraddr, sizeof(peeraddr))) {
			printf("[오류] 잘못된 데이터입니다!\n");
			break;
		}

		// 받은 데이터 출력
		buf[retval] = '\0';
		printf("[UDP 클라이언트] %d바이트를 받았습니다.\n", retval);
		printf("[받은 데이터] %s\n", buf);
	}

	// 소켓 닫기
	close(sock);
	return 0;
}