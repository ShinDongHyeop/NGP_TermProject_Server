#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <time.h>
#include "Player.h"
#include "Bullet.h"

using namespace std;

#define SERVERPORT		5000

#define MAX_PLAYER		3

enum GameState {LOGIN, RUNNING, END};
enum PlayerState{WAIT, READY, START, PLAY, DIE, RESPAWN};

CRITICAL_SECTION cs;

void err_quit(char *msg) {
	LPVOID IpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&IpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)IpMsgBuf, msg, MB_ICONERROR);
	LocalFree(IpMsgBuf);
	exit(1);
}

void err_display(char* msg) {
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while (left > 0) {
		received = recv(s, ptr, left, flags);
		if (received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if (received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

int player_Count = 0;

int player_State[3]{ WAIT, WAIT, WAIT };

vector<Player*> players;

bool readyCheck() {
	for (int i = 0; i < 3; ++i)
		if (player_State[i] == WAIT)
			return false;
	return true;
}

void initAllData(){
	for (int i = 0; i < MAX_PLAYER; ++i) {
		srand(time(NULL));
		float random_X = (float)rand() / RAND_MAX * 2000.0f;
		float random_Y = (float)rand() / RAND_MAX * 2000.0f;
		players.push_back(new Player(random_X, random_Y, 100));
	}
}

DWORD WINAPI myGameThread(LPVOID arg) {
	SOCKET client_sock = (SOCKET)arg;
	int retval;
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR *)&clientaddr, &addrlen);
	int playerCode = player_Count;
	player_Count++;
	int type;
	while (1) {
		retval = recv(client_sock, (char*)&type, sizeof(int), 0);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0)
			break;
		switch (type)
		{
		case LOGIN:
			int state;
			retval = recv(client_sock, (char*)&state, sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			player_State[playerCode] = state;
			state = player_State[playerCode];

			EnterCriticalSection(&cs);
				printf("<\t");
				for (int i = 0; i < 3; ++i) {
					if (player_State[i] == WAIT)
						printf("WAIT\t");
					else if (player_State[i] == READY)
						printf("READY\t");
				}
				printf(">\n");
			LeaveCriticalSection(&cs);

			retval = send(client_sock, (char*)&state, sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
			break;
		case RUNNING:
			break;
		case END:
			break;
		}
	}
	closesocket(client_sock);
	return 0;
}

int main(int argc, char *argv[])
{
	int retval;
	InitializeCriticalSection(&cs);
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// socket()
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	SOCKADDR_IN serveraddr;
	ZeroMemory(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = bind(listen_sock, (SOCKADDR *)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;
	SOCKADDR_IN clientaddr;
	int addrlen;
	HANDLE hThread;

	while (1) {
		// accept()
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		// 접속한 클라이언트 정보 출력
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

		// 스레드 생성
		hThread = CreateThread(NULL, 0, myGameThread,
			(LPVOID)client_sock, 0, NULL);
		if (hThread == NULL) { closesocket(client_sock); }
		else { CloseHandle(hThread); }
	}
	DeleteCriticalSection(&cs);
	// closesocket()
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}