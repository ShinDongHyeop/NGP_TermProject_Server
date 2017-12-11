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
//HANDLE hRecvEvent[3];
//HANDLE hSendEvent[3];
HANDLE clientEvent[3];

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
clock_t current_time[3] = { clock(), clock(), clock() };

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
			if (players[code]->collBullet(10.0f))
				player_State[code] = DIE;
		}
	}
}

void respawnPlayer(int code) {
	srand(time(NULL));
	float random_X = (float)rand() / RAND_MAX * 2000.0f;
	float random_Y = (float)rand() / RAND_MAX * 2000.0f;
	players[code]->respawn(random_X, random_Y, PLAYER_HP);
}

DWORD WINAPI myGameThread(LPVOID arg) {
	SOCKET client_sock = (SOCKET)arg;
	client_Socks[player_Count] = (SOCKET)arg;
	SOCKADDR_IN clientaddr;
	int addrlen = sizeof(clientaddr);
	getpeername(client_sock, (SOCKADDR *)&clientaddr, &addrlen);
	int frame_time;
	float start_time, respawn_time;
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
			retval = recv(client_sock, (char*)&player_State[playerCode], sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}

			if (readyCheck()) {
				player_State[playerCode] = START;
				retval = send(client_sock, (char*)&player_State[playerCode], sizeof(int), 0);
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
				current_time[playerCode] = clock();
			}
			else {
				retval = send(client_sock, (char*)&player_State[playerCode], sizeof(int), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send()");
					break;
				}
			}
			break;
		case RUNNING:
			retval = WaitForSingleObject(clientEvent[(playerCode + 1) % 3], INFINITE);
			if (retval != WAIT_OBJECT_0) break;
			EnterCriticalSection(&cs);
			ResetEvent(clientEvent[(playerCode + 1) % 3]);
			frame_time= clock() - current_time[playerCode];
			
			current_time[playerCode] += frame_time;
			//printf("player %d frame : %d", playerCode, 1000 / frame_time);
			retval = recv(client_sock, (char*)&player_State[playerCode], sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			switch (player_State[playerCode])
			{
			case START:
				retval = send(client_sock, (char*)&frame_time, sizeof(int), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send()");
					break;
				}
				retval = recv(client_sock, (char*)&start_time, sizeof(float), 0);
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					break;
				}
				if (start_time <= 0.0f)
					player_State[playerCode] = PLAY;
				break;
			case PLAY:
				retval = recvn(client_sock, (char*)&clientBuf[playerCode], sizeof(ClientBuf), 0);
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					break;
				}

				changePlayerData(playerCode, frame_time);

				collisionObjects(playerCode);

				setPlayerBuf();
				setBulletBuf();

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
				break;
			case DIE:
				setPlayerBuf();
				setBulletBuf();

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
				break;
			case RESPAWN:
				retval = send(client_sock, (char*)&frame_time, sizeof(int), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send()");
					break;
				}
				retval = recv(client_sock, (char*)&respawn_time, sizeof(float), 0);
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					break;
				}
				if (respawn_time <= 0.0f) {
					player_State[playerCode] = PLAY;
					respawnPlayer(playerCode);
				}
				setPlayerBuf();
				setBulletBuf();

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
				break;
			}
			retval = send(client_sock, (char*)&player_State[playerCode], sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send()");
				break;
			}
			SetEvent(clientEvent[playerCode]);
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
	clientEvent[0] = CreateEvent(NULL, TRUE, FALSE, NULL);
	clientEvent[1] = CreateEvent(NULL, TRUE, TRUE, NULL);
	clientEvent[2] = CreateEvent(NULL, TRUE, FALSE, NULL);

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
			int opt_val = TRUE;
			setsockopt(client_sock, IPPROTO_TCP,TCP_NODELAY, (char*)&opt_val, sizeof(int));
			// 스레드 생성
			hThread = CreateThread(NULL, 0, myGameThread,
				(LPVOID)client_sock, 0, NULL);
			if (hThread == NULL) { closesocket(client_sock); }
			else { CloseHandle(hThread); }
		}
	}
	DeleteCriticalSection(&cs);
	for (int i = 0; i < 3; ++i)
		CloseHandle(clientEvent[i]);
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}