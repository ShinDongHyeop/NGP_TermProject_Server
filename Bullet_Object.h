class Bullet {
	int owner;
	float start_X, start_Y;
	float real_X, real_Y;
	float vector_X, vector_Y;
	float coll_Box[4];
public:
	Bullet(
		int init_owner, 
		float init_X, float init_Y, 
		float p_LookX, float p_LookY);

	float getRealX() { return real_X; }
	float getRealY() { return real_Y; }
	float* getCollBox() { return coll_Box; }

	int getOwner() { return owner; }

	/////////////////////////////////////////

	bool overRange();

	void update(int frame_time);
};