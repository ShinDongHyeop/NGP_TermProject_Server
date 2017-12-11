
class Player {
	float hp;
	float speed;
	float real_X, real_Y;
	float look_X, look_Y;
	float move_State[2]{};
	int shoot_State;
	float shoot_CollTime;
	float coll_Box[4];
	int kill, death;
public:
	Player(float pX, float pY, float pHP);

	float getRealX() { return real_X; }
	float getRealY() { return real_Y; }
	float getLookX() { return look_X; }
	float getLookY() { return look_Y; }
	float getHP() { return hp; }
	float* getCollBox() { return coll_Box; }
	float move(float min, float x, float v, float max);
	int getKill() { return kill; }
	int getDeath() { return death; }
	bool shoot();

	void changeMove(int* moveState);
	void changeShootState(int state);
	void changeLookXY(float lookX, float lookY);
	void update(int frame_time);
	bool collBullet(float damage);
	void respawn(float pX, float pY, float pHP);
	void dying() { death++; }
	void killing() { kill++; }
};