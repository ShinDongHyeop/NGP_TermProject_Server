#include <math.h>

#define SERVERPORT		5000

#define MAX_PLAYER		3
#define MAX_BULLET		100
#define PB_SIZE			(sizeof(PlayerBuf) * MAX_PLAYER)

#define PLAYER_HP		100
#define PLAYER_SIZE		10
#define PLAYER_SPEED	20.0f

#define BULLET_RANGE	300
#define BULLET_SPEED	100.0f

#define KPH_TO_MPS		(1000.0f / 3600.0f)
#define METTER_TO_PIXEL	40.0f

enum ShootState { NO_SHOOT, SHOOT };
enum GameState { LOGIN, RUNNING, END };
enum PlayerState { WAIT, READY, START, PLAY, DIE, RESPAWN };