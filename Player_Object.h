class Player{
	float hp;
	float real_X, real_Y;
	float look_X, look_Y;
	float shoot_CollTime;
	float coll_Box[4];
	int move_State[2];
	int shoot_State;
	int kill, death;
	int id;
public:
	Player(int code, float init_X, float init_Y, float init_HP);

	int getID() { return id; }
	float getRealX() { return real_X; }
	float getRealY() { return real_Y; }
	float getLookX() { return look_X; }
	float getLookY() { return look_Y; }
	float getHP() { return hp; }
	float* getCollBox() { return coll_Box; }
	
	int getKill() { return kill; }
	int getDeath() { return death; }

	void dying() { death++; }
	void killing() { kill++; }

	////////////////////////////////////////////

	float move(float min, float pos, float veloc, float max);

	bool shoot();
	bool collBullet(float damage);

	void reset(float pX, float pY, float pHP);
	void eatItem();
	void changeMove(int* moveState);
	void changeShootState(int state);
	void changeLookXY(float lookX, float lookY);
	void respawn(float pX, float pY, float pHP);
	void update(int frame_time);
};