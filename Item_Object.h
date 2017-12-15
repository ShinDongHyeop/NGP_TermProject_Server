class Item {
	float real_X, real_Y;
	float coll_Box[4];
	int respawn_time;
	int state;
public:
	Item(float realX, float realY);

	float getRealX() { return real_X; }
	float getRealY() { return real_Y; }
	float* getCollBox() { return coll_Box; }
	int getState() { return state; }

	void eating();
	
	bool respawnOK();

	void setRespawn(float realX, float realY);
	void update(int frame_time);
};