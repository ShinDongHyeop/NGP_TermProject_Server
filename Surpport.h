#pragma comment(lib, "ws2_32")

#include <WinSock2.h>
#include <iostream>
#include <vector>
#include <list>
#include <time.h>
#include <math.h>
#include <algorithm>

using namespace std;

#define SERVERPORT		5000

#define MAX_PLAYER		10
#define MAX_BULLET		100
#define PB_SIZE			(sizeof(PlayerBuf) * MAX_PLAYER)

#define PLAYER_HP		100
#define PLAYER_SIZE		10
#define PLAYER_SPEED	20.0f

#define BULLET_RANGE	300
#define BULLET_SPEED	100.0f
#define BULLET_SIZE		3.0f

#define MAX_ITEM		5
#define ITEM_SIZE		8
#define ITEM_RESPAWN_TIME	5000
#define RECOVERY		20

#define KPH_TO_MPS		(1000.0f / 3600.0f)
#define METTER_TO_PIXEL	40.0f

#define ROOM_OWNER		0

#define MAX_KILL		10
#define NULL_PLAYER		999

// 상태 변수

enum CollBox { LEFT, RIGHT, BOTTOM, TOP };
enum ShootState { NO_SHOOT, SHOOT };
enum GameState { LOGIN, RUNNING, END };
enum PlayerState { NONE, WAIT, READY, START, PLAY, DIE };
enum StartOK { ON, OFF, STARTOK };
enum ItemState { ITEM_EXIST, ITEM_NONE };

// 충돌체크 함수

template<class ObjectA, class ObjectB>
bool collision(ObjectA a, ObjectB b) {
	float* collA = (*a)->getCollBox();
	float* collB = (*b)->getCollBox();
	if (collA[LEFT] > collB[RIGHT])		return false;
	if (collA[RIGHT] < collB[LEFT])		return false;
	if (collA[BOTTOM] > collB[TOP])		return false;
	if (collA[TOP] < collB[BOTTOM])		return false;
	return true;
}

// 송수신 서포트

