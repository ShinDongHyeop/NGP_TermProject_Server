#define PLAYER_HP		100
#define PLAYER_SIZE		10
#define PLAYER_SPEED	20.0f
#define KPH_TO_MPS		(1000.0f / 3600.0f)
#define METTER_TO_PIXEL	40.0f

class Player {
	float hp;
	float speed = PLAYER_SPEED;
	float real_X, real_Y;
	float move_State[2]{};
public:
	Player(float pX, float pY, float pHP);

	float getRealX() { return real_X; }
	float getRealY() { return real_Y; }
	float getHP() { return hp; }

	float move(float min, float x, float v, float max);

	void changeMove(int* moveState);
	void update(float frame_time);
};