#include <memory>

#include "Surpport.h"
#include "Player_Object.h"
#include "Bullet_Object.h"
#include "Buffer_Object.h"
#include "Item_Object.h"

CRITICAL_SECTION cs;

int retval;
int game_State = LOGIN;
int start_State = OFF;
int player_Count = 0;
int winner = NULL_PLAYER;
int players_State[MAX_PLAYER] = { NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE, NONE };
int all_Count[2] {};
int room_owner = ROOM_OWNER;
int max_Kill = MAX_KILL;

list<unique_ptr<Player>> players;
list<unique_ptr<Bullet>> bullets;
vector<unique_ptr<Item>> items;

PlayerBuf players_Buf[MAX_PLAYER];
BulletBuf bullets_Buf[MAX_BULLET];
ItemBuf items_Buf[MAX_ITEM];

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

bool all_Ready_Check() {
	if (players.size() < 2)	return false;
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (players_State[i] == WAIT)
			return false;
	}
	return true;
}

bool ready_Check() {
	if (players.size() < 2)	return false;
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (i == room_owner)
			continue;
		if (players_State[i] == WAIT)
			return false;
	}
	return true;
}

int setPlayersBuf() {
	int num = 0;
	for (auto i = players.begin(); i != players.end(); ++i) {
		players_Buf[num].real_X = (*i)->getRealX();	players_Buf[num].real_Y = (*i)->getRealY();
		players_Buf[num].look_X = (*i)->getLookX();	players_Buf[num].look_Y = (*i)->getLookY();
		players_Buf[num].id = (*i)->getID();			players_Buf[num].hp = (*i)->getHP();
		players_Buf[num].kill = (*i)->getKill();		players_Buf[num].death = (*i)->getDeath();
		++num;
	}
	return num;
}

int setBulletsBuf() {
	int num = 0;
	for (auto i = bullets.begin(); i != bullets.end(); ++i) {
		bullets_Buf[num].real_X = (*i)->getRealX();
		bullets_Buf[num].real_Y = (*i)->getRealY();
		++num;
	}
	return num;
}

void setItemsBuf() {
	int num = 0;
	for (auto i = items.begin(); i != items.end(); ++i) {
		items_Buf[num].realX = (*i)->getRealX();
		items_Buf[num].realY = (*i)->getRealY();
		items_Buf[num].state = (*i)->getState();
		++num;
	}
}

void updatePlayerData(int code, ClientBuf cb, clock_t frame_time) {
	auto player = find_if(players.begin(), players.end(), [code](unique_ptr<Player>& a) {
		return a->getID() == code;
	});
	
	(*player)->changeLookXY(cb.look_X, cb.look_Y);
	(*player)->changeShootState(cb.shoot_State);
	(*player)->changeMove(cb.move_State);
	(*player)->update((int)frame_time);

	if ((*player)->shoot()) {
		bullets.push_back(
			unique_ptr<Bullet>(new Bullet(
				(*player)->getID(),
				(*player)->getRealX(), (*player)->getRealY(),
				(*player)->getLookX(), (*player)->getLookY()
			))
		);
	}
	for (auto i = bullets.begin(); i != bullets.end(); ++i) {
		if ((*i)->getOwner() == (*player)->getID()) {
			(*i)->update(frame_time);
			if ((*i)->overRange())
				bullets.remove((*i));
		}
	}
}

void updateItems(int ft) {
	for (auto item = items.begin(); item != items.end(); ++item) {
		(*item)->update(ft);
		if ((*item)->respawnOK()){
			(*item)->setRespawn(
				(float)rand() / RAND_MAX * 2000.0f,
				(float)rand() / RAND_MAX * 2000.0f
			);
		}
	}
}

void collisionObjects(int code) {
	auto m_player = find_if(players.begin(), players.end(), [code](unique_ptr<Player>& a) {
		return a->getID() == code;
	});
	for (auto item = items.begin(); item != items.end(); ++item) {
		if (collision(item, m_player) && (*item)->getState() != ITEM_NONE) {
			(*item)->eating();
			(*m_player)->eatItem();
		}
	}
	for (auto bullet = bullets.begin(); bullet != bullets.end(); ++bullet) {
		if ((*bullet)->getOwner() == code) {
			for (auto player = players.begin(); player != players.end(); ++player) {
				if (((*player)->getID() != (*bullet)->getOwner()) && (collision(bullet, player)) && players_State[(*player)->getID()] != DIE) {
					if ((*player)->collBullet(10.0f)){
						(*player)->dying();
						(*m_player)->killing();
						players_State[(*player)->getID()] = DIE;
						bullets.remove((*bullet));
						break;
					}
					bullets.remove((*bullet));
					break;
				}
			}
		}
	}
}

int nextPlayerCode(int code, int next) {
	if (players.size() == 1 || code == next)
		return code;
	if (next >= player_Count)
		return nextPlayerCode(code, 0);
	if (players_State[next] == DIE || players_State[next] == NONE)
		return nextPlayerCode(code, next + 1);
	return next;
}

int nextRoomOwner(int code, int next) {
	if (next >= 10)
		next = 0;
	if (player_Count < 2)
		return code;
	else if (players_State[next] != NONE)
		return next;
	else
		return nextRoomOwner(code, next + 1);
}

void respawnPlayer(int code) {
	auto player = find_if(players.begin(), players.end(), [code](unique_ptr<Player>& a) {
		return a->getID() == code;
	});
	(*player)->respawn(
		(float)rand() / RAND_MAX * 2000.0f, 
		(float)rand() / RAND_MAX * 2000.0f,
		PLAYER_HP
	);
}

int gameEndCheck() {
	for (auto i = players.begin(); i != players.end(); ++i) {
		if ((*i)->getKill() >= max_Kill)
			return (*i)->getID();
	}
	return NULL_PLAYER;
}

void gameReset() {
	start_State = OFF;
	for (auto player = players.begin(); player != players.end(); ++player) {
		(*player)->reset(
			(float)rand() / RAND_MAX * 2000.0f,
			(float)rand() / RAND_MAX * 2000.0f,
			PLAYER_HP
		);
	}
}

int nextPlayerCode() {
	for (int i = 0; i < MAX_PLAYER; ++i) {
		if (players_State[i] == NONE)
			return i;
	}
	return NULL_PLAYER;
}

void initItems() {
	for (int i = 0; i < MAX_ITEM; ++i) {
		items.push_back(
			unique_ptr<Item>(new Item(
				(float)rand() / RAND_MAX * 2000.0f,
				(float)rand() / RAND_MAX * 2000.0f
			))
		);
	}
}

DWORD WINAPI myGameThread(LPVOID arg) {
	SOCKET client_sock = (SOCKET)arg;
	clock_t currunt_time = clock();
	clock_t frame_time;
	int player_Code = nextPlayerCode();
	int ready_State = 0;
	int client_Game_State = game_State;
	int game_Start_time = 0;
	int respawn_time = 0;
	int restart_time = 0;
	int pbsize = 0, bbsize = 0, len = 0;
	ClientBuf client_Buf;
	player_Count++;
	srand((int)clock());
	players_State[player_Code] = WAIT;
	players.push_back(unique_ptr<Player>(new Player(
		player_Code,
		(float)rand() / RAND_MAX * 2000.0f,
		(float)rand() / RAND_MAX * 2000.0f,
		PLAYER_HP
	)));
	retval = send(client_sock, (char*)&player_Code, sizeof(int), 0);
	if (retval == SOCKET_ERROR) {
		err_display("send()");
		goto error;
	}
	while (1) {
		frame_time = clock() - currunt_time;
		if (frame_time < 25) {
			Sleep(25 - frame_time);
			frame_time = clock() - currunt_time;
		}
		currunt_time += frame_time;
		retval = send(client_sock, (char*)&room_owner, sizeof(int), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send1()");
			goto error;
		}
		retval = send(client_sock, (char*)&client_Game_State, sizeof(int), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send1()");
			goto error;
		}
		len = (int)sizeof(int)*MAX_PLAYER;
		retval = send(client_sock, (char*)&players_State, len, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send2()");
			goto error;
		}
		switch (client_Game_State)
		{
		case LOGIN:
			retval = recv(client_sock, (char*)&ready_State, sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				goto error;
			}
			if (ready_State == 1)
				players_State[player_Code] = READY;
			else if (ready_State == 0)
				players_State[player_Code] = WAIT;

			if (player_Code == room_owner) {
				if (all_Ready_Check()) {
					game_State = RUNNING;
					start_State = STARTOK;
				}
				else if (ready_Check())
					start_State = ON;
				else
					start_State = OFF;
			}
			retval = send(client_sock, (char*)&start_State, sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send3()");
				goto error;
			}
			if (start_State == STARTOK) {
				players_State[player_Code] = START;
				client_Game_State = game_State;
				all_Count[0] = setPlayersBuf();
				setItemsBuf();
				retval = send(client_sock, (char*)&all_Count[0], sizeof(int), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send4()");
					goto error;
				}
				pbsize = (int)sizeof(PlayerBuf) * all_Count[0];
				retval = send(client_sock, (char*)&players_Buf, pbsize, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send5()");
					goto error;
				}
				len = (int)sizeof(ItemBuf)*MAX_ITEM;
				retval = send(client_sock, (char*)&items_Buf, len, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send5()");
					goto error;
				}
			}
			break;
		case RUNNING:
			switch (players_State[player_Code])
			{
			case START:
				game_Start_time += (int)frame_time;
				retval = send(client_sock, (char*)&game_Start_time, sizeof(int), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send6()");
					goto error;
				}
				if (game_Start_time >= 5000) {
					players_State[player_Code] = PLAY;
					game_Start_time = 0;
				}
				if (player_Code == room_owner) {
					if (player_Count < 2) {
						game_Start_time = 0;
						gameReset();
						bullets.clear();
						game_State = LOGIN;
					}
				}
				if (game_State == LOGIN) {
					client_Game_State = game_State;
					players_State[player_Code] = WAIT;
				}
				break;
			case PLAY:
				retval = recv(client_sock, (char*)&client_Buf, sizeof(ClientBuf), 0);
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					goto error;
				}
				EnterCriticalSection(&cs);
				updatePlayerData(player_Code, client_Buf, frame_time);
				collisionObjects(player_Code);
				if (player_Code == room_owner) {
					updateItems((int)frame_time);
				}
				all_Count[0] = setPlayersBuf();
				all_Count[1] = setBulletsBuf();
				setItemsBuf();
				retval = send(client_sock, (char*)&all_Count, sizeof(all_Count), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send7()");
					goto error;
				}
				pbsize = (int)sizeof(PlayerBuf) * all_Count[0];
				retval = send(client_sock, (char*)&players_Buf, pbsize, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send8()");
					goto error;
				}
				bbsize = (int)sizeof(BulletBuf) * all_Count[1];
				retval = send(client_sock, (char*)&bullets_Buf, bbsize, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send9()");
					goto error;
				}
				len = (int)sizeof(ItemBuf)*MAX_ITEM;
				retval = send(client_sock, (char*)&items_Buf, len, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send5()");
					goto error;
				}
				if (player_Code == room_owner) {
					winner = gameEndCheck();
					if (winner != NULL_PLAYER) {
						gameReset();
						bullets.clear();
						game_State = END;
					}
					else if (player_Count < 2) {
						gameReset();
						bullets.clear();
						game_State = LOGIN;
					}
				}
				if (game_State == END) {
					client_Game_State = game_State;
					players_State[player_Code] = WAIT;
				}
				if (game_State == LOGIN) {
					client_Game_State = game_State;
					players_State[player_Code] = WAIT;
				}
				LeaveCriticalSection(&cs);
				break;
			case DIE:
				respawn_time += (int)frame_time;
				retval = send(client_sock, (char*)&respawn_time, sizeof(int), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send10()");
					goto error;
				}
				if (respawn_time >= 5000) {
					players_State[player_Code] = PLAY;
					respawnPlayer(player_Code);
					respawn_time = 0;
				}
				EnterCriticalSection(&cs);
				if (player_Code == room_owner) {
					updateItems((int)frame_time);
				}
				all_Count[0] = setPlayersBuf();
				all_Count[1] = setBulletsBuf();
				setItemsBuf();
				retval = send(client_sock, (char*)&all_Count, sizeof(all_Count), 0);
				if (retval == SOCKET_ERROR) {
					err_display("send11()");
					goto error;
				}
				pbsize = (int)sizeof(PlayerBuf) * all_Count[0];
				retval = send(client_sock, (char*)&players_Buf, pbsize, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send12()");
					goto error;
				}
				bbsize = (int)sizeof(BulletBuf) * all_Count[1];
				retval = send(client_sock, (char*)&bullets_Buf, bbsize, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send13()");
					goto error;
				}
				len = (int)sizeof(ItemBuf)*MAX_ITEM;
				retval = send(client_sock, (char*)&items_Buf, len, 0);
				if (retval == SOCKET_ERROR) {
					err_display("send5()");
					goto error;
				}
				if (player_Code == room_owner) {
					winner = gameEndCheck();
					if (winner != NULL_PLAYER) {
						gameReset();
						bullets.clear();
						game_State = END;
					}
					else if (player_Count < 2) {
						respawn_time = 0;
						gameReset();
						bullets.clear();
						game_State = LOGIN;
					}
				}
				if (game_State == END) {
					client_Game_State = game_State;
					players_State[player_Code] = WAIT;
				}
				if (game_State == LOGIN) {
					client_Game_State = game_State;
					players_State[player_Code] = WAIT;
				}
				LeaveCriticalSection(&cs);
				break;
			}
			break;
		case END:
			restart_time += frame_time;
			retval = send(client_sock, (char*)&winner, sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send15()");
				goto error;
			}
			retval = send(client_sock, (char*)&restart_time, sizeof(int), 0);
			if (retval == SOCKET_ERROR) {
				err_display("send16()");
				goto error;
			}
			if (restart_time >= 5000) {
				restart_time = 0;
				if (player_Code == room_owner) {
					game_State = LOGIN;
				}
				if (game_State == LOGIN) {
					winner = NULL_PLAYER;
					client_Game_State = game_State;
				}
			}
			break;
		}
	}
error:
	if (player_Code == room_owner) {
		room_owner = nextRoomOwner(player_Code, player_Code + 1);
	}
	players_State[player_Code] = NONE;
	players.remove_if([player_Code](unique_ptr<Player>& a) {
		return a->getID() == player_Code;
	});
	player_Count--;
	closesocket(client_sock);
	return 0;
}

int main(int argc, char *argv[]) {
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	InitializeCriticalSection(&cs);
	initItems();
	cout << "승리조건 킬 수 : ";
	cin >> max_Kill;
	cout << "게임을 시작합니다!!!" << endl;
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
	SOCKET client_sock[MAX_PLAYER];
	SOCKADDR_IN clientaddr;
	int addrlen;
	HANDLE hThread;
	int num = 0;
	while (1) {
		// accept()
		if (player_Count == 10)
			continue;
		addrlen = sizeof(clientaddr);
		client_sock[num] = accept(listen_sock, (SOCKADDR *)&clientaddr, &addrlen);
		if (client_sock[num] == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}
		BOOL opt_val = TRUE;
		setsockopt(client_sock[num], IPPROTO_TCP, TCP_NODELAY, (char*)&opt_val, sizeof(opt_val));

			// 접속한 클라이언트 정보 출력
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));


		if (game_State == LOGIN) {
			// 스레드 생성
			hThread = CreateThread(NULL, 0, myGameThread,
				(LPVOID)client_sock[num], 0, NULL);
			if (hThread == NULL) { closesocket(client_sock[num]); }
			else { CloseHandle(hThread); }
			++num;
		}
		else
			closesocket(client_sock[num]);
	}
	DeleteCriticalSection(&cs);
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}