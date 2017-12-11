#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <time.h>
#include <list>
#include "Player.h"
#include "Bullet.h"
#include "Buffer.h"
#include "suf.h"

using namespace std;

CRITICAL_SECTION cs;
HANDLE hRecvEvent[3];
HANDLE hSendEvent[3];

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
list<Bullet*> bullets;
PlayerBuf playersBuf[3];
BulletBuf bulletsBuf[MAX_BULLET];
ClientBuf clientBuf[3];
SOCKET client_Socks[3];

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
		playersBuf[i].real_X = players[i]->getRealX();
		playersBuf[i].real_Y = players[i]->getRealY();
		playersBuf[i].look_X = players[i]->getLookX();
		playersBuf[i].look_Y = players[i]->getLookY();
		playersBuf[i].hp = players[i]->getHP();
		playersBuf[i].state = player_State[i];
	}
}

void setBulletBuf() {
	int i = 0;
	for (auto b : bullets) {
		bulletsBuf[i].real_X = b->getRealX();
		bulletsBuf[i].real_Y = b->getRealY();
		++i;
	}
}

void changePlayerData(int code, int frame_time) {
	players[code]->changeMove(clientBuf[code].move_State);
	players[code]->changeShootState(clientBuf[code].shoot_State);
	players[code]->changeLookXY(clientBuf[code].look_X, clientBuf[code].look_Y);
	players[code]->update(frame_time);
	if (players[code]->shoot()) {
		bullets.push_back(
			new Bullet(
				code,
				players[code]->getRealX(), players[code]->getRealY(),
				clientBuf[code].look_X, clientBuf[code].look_Y
			)
		);
	}
	for (auto b : bullets) {
		b->update(frame_time);
		if (b->overRange())
			bullets.remove(b);
	}
}

void collisionObjects(int code) {
	for (auto d : bullets) {
		if (collision(d, players[code]) && (code != d->getOwner())) {
			bullets.remove(d);
			players[code]->collBullet(10.0f);
		}
	}
}

DWORD WINAPI myGameThread(LPVOID arg) {
	SOCKET client_sock = (SOCKET)arg;
	client_Socks[player_Count] = (SOCKET)arg;
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR *)&clientaddr, &addrlen);
	int frame_time;
	int retval;
	int playerCode = player_Count;
	int bullet_count;
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
			break;
		case RUNNING:
			retval = recvn(client_sock, (char*)&clientBuf[playerCode], sizeof(ClientBuf), 0);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			retval = WaitForMultipleObjects(3, hSendEvent, TRUE, INFINITE);
			if (retval == WAIT_FAILED) break;

			changePlayerData(playerCode, frame_time);

			EnterCriticalSection(&cs);
			collisionObjects(playerCode);
			LeaveCriticalSection(&cs);

			setPlayerBuf();
			setBulletBuf();

			EnterCriticalSection(&cs);
			SetEvent(hRecvEvent[0]);
			SetEvent(hRecvEvent[1]);
			SetEvent(hRecvEvent[2]);
			LeaveCriticalSection(&cs);

			retval = WaitForMultipleObjects(3, hRecvEvent, TRUE, INFINITE);
			if (retval == WAIT_FAILED) break;

			retval = send(client_sock, (char*)&playersBuf, PB_SIZE, 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
			bullet_count = bullets.size();
			retval = send(client_sock, (char*)&bullet_count, sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
			}
			retval = send(client_sock, (char*)&bulletsBuf, sizeof(BulletBuf)*bullet_count, 0);
			if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
			}
			EnterCriticalSection(&cs);
			SetEvent(hSendEvent[0]);
			SetEvent(hSendEvent[1]);
			SetEvent(hSendEvent[2]);
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
	hRecvEvent[0] = CreateEvent(NULL, FALSE, FALSE, NULL);
	hRecvEvent[1] = CreateEvent(NULL, FALSE, FALSE, NULL);
	hRecvEvent[2] = CreateEvent(NULL, FALSE, FALSE, NULL);
	hSendEvent[0] = CreateEvent(NULL, FALSE, TRUE, NULL);
	hSendEvent[1] = CreateEvent(NULL, FALSE, TRUE, NULL);
	hSendEvent[2] = CreateEvent(NULL, FALSE, TRUE, NULL);

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
		if (player_Count < MAX_PLAYER - 1) {
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
			//SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
		}
	}
	DeleteCriticalSection(&cs);
	// closesocket()
	for (int i = 0; i < 3; ++i)
		CloseHandle(hRecvEvent[i]);
	for (int i = 0; i < 3; ++i)
		CloseHandle(hSendEvent[i]);
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}