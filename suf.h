#include <math.h>

#define SERVERPORT		5000

#define MAX_PLAYER		3
#define MAX_BULLET		100
#define PB_SIZE			(sizeof(PlayerBuf) * MAX_PLAYER)

#define PLAYER_HP		100
#define PLAYER_SIZE		10
#define PLAYER_SPEED	20.0f
#define PLAYER_SIZE		10.0f

#define BULLET_RANGE	300
#define BULLET_SPEED	100.0f
#define BULLET_SIZE		3.0f

#define MAX_ITEM		50
#define RECOVERY		30.0f

#define KPH_TO_MPS		(1000.0f / 3600.0f)
#define METTER_TO_PIXEL	40.0f

enum CollBox { LEFT, RIGHT, BOTTOM, TOP };
enum ShootState { NO_SHOOT, SHOOT };
enum GameState { LOGIN, RUNNING, END };
enum PlayerState { WAIT, READY, START, PLAY, DIE, RESPAWN };

template<class ObjectA, class ObjectB>
bool collision(ObjectA* a, ObjectB* b) {
	float* collA = a->getCollBox();
	float* collB = b->getCollBox();
	if (collA[LEFT] > collB[RIGHT])		return false;
	if (collA[RIGHT] < collB[LEFT])		return false;
	if (collA[BOTTOM] > collB[TOP])		return false;
	if (collA[TOP] < collB[BOTTOM])		return false;
	return true;
}