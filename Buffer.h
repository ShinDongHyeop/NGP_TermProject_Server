#pragma pack(1)

struct PlayerBuf {
	float real_X, realY;
	float look_X, look_Y;
	int state, code;
};

struct BulletBuf {
	float draw_X, draw_Y;
};

struct ClientBuf {
	int move_State[2];
	int shoot_State;
	float mouse_X, mouse_Y;
};

#pragma pack()