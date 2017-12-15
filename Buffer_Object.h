#pragma pack(1)

struct ClientBuf {
	int move_State[2];
	int shoot_State;
	float look_X, look_Y;
};

struct PlayerBuf {
	float real_X, real_Y;
	float look_X, look_Y;
	float hp;
	int id;
	int kill, death;
};

struct BulletBuf {
	float real_X, real_Y;
};

struct ItemBuf {
	float realX, realY;
	int state;
};

#pragma pack()