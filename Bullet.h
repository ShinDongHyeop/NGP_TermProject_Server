
class Bullet {
	float owner;
	float start_X, start_Y;
	float real_X, real_Y;
	float vector_X, vector_Y;
	float range;
	float coll_Box[4];
public:
	Bullet(float pOwner, float realX, float realY, float lookX, float lookY);

	float getRealX() { return real_X; }
	float getRealY() { return real_Y; }
	float* getCollBox() { return coll_Box; }
	int getOwner() { return owner; }

	bool overRange();

	void update(int frame_time);
};