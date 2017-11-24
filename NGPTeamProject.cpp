#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <time.h>
#include "Player.h"
#include "Bullet.h"
#include "Buffer.h"

using namespace std;

#define SERVERPORT		5000

#define MAX_PLAYER		3
#define PB_SIZE			(sizeof(PlayerBuf) * MAX_PLAYER)

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
PlayerBuf playersBuf[3];
ClientBuf clientBuf[3];

bool readyCheck() {
	for (int i = 0; i < 3; ++i)
		if (player_State[i] == WAIT)
			return false;
	return true;
}

void initAllData(){
	srand(time(NULL));
	for (int i = 0; i < MAX_PLAYER; ++i) {
		float random_X = (float)rand() / RAND_MAX * 2000.0f;
		float random_Y = (float)rand() / RAND_MAX * 2000.0f;
		players.push_back(new Player(random_X, random_Y, PLAYER_HP));
	}
}

void setPlayerBuf() {
	for (int i = 0; i < MAX_PLAYER; ++i) {
		playersBuf[i].code = i;
		playersBuf[i].real_X = players[i]->getRealX();
		playersBuf[i].real_Y = players[i]->getRealY();
		playersBuf[i].look_X = players[i]->getLookX();
		playersBuf[i].look_Y = players[i]->getLookY();
		playersBuf[i].hp = players[i]->getHP();
		playersBuf[i].state = player_State[i];
	}
}

void changePlayerData(int code, int frame_time) {
	players[code]->changeMove(clientBuf[code].move_State);
	players[code]->update(frame_time);
}

DWORD WINAPI myGameThread(LPVOID arg) {
	SOCKET client_sock = (SOCKET)arg;
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR *)&clientaddr, &addrlen);
	int frame_time;
	int retval;
	int playerCode = player_Count;
	retval = send(client_sock, (char*)&playerCode, sizeof(int), 0);
	if (retval == SOCKET_ERROR) {
		err_display("send()");
	}
	player_Count++;
	int game_State;
	while (1) {
		retval = recv(client_sock, (char*)&frame_time, sizeof(int), 0);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		retval = recv(client_sock, (char*)&game_State, sizeof(int), 0);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0)
			break;
		switch (game_State)
		{
		case LOGIN:
			int state;
			retval = recv(client_sock, (char*)&state, sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			player_State[playerCode] = state;

			EnterCriticalSection(&cs);
			if (readyCheck()) {
				player_State[playerCode] = START;
				state = player_State[playerCode];
				retval = send(client_sock, (char*)&state, sizeof(int), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send()");
					break;
				}
				initAllData();
				setPlayerBuf();
				retval = send(client_sock, (char*)&playersBuf, PB_SIZE, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send()");
					break;
				}
			}
			else {
				state = player_State[playerCode];
				retval = send(client_sock, (char*)&state, sizeof(int), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send()");
					break;
				}
			}
			LeaveCriticalSection(&cs);
			break;
		case RUNNING:
			EnterCriticalSection(&cs);
			retval = recvn(client_sock, (char*)&clientBuf[playerCode], sizeof(ClientBuf), 0);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			changePlayerData(playerCode, frame_time);
			setPlayerBuf();
			retval = send(client_sock, (char*)&playersBuf, PB_SIZE, 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
			LeaveCriticalSection(&cs);
			break;
		case END:
			break;
		}
	}
	player_Count--;
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
		if (player_Count <= MAX_PLAYER) {
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
	}
	DeleteCriticalSection(&cs);
	// closesocket()
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}