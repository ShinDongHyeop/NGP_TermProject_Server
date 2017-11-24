
class Bullet {
	float start_X, start_Y;
	float real_X, real_Y;
	float vector_X, vector_Y;
	float range;
public:
	Bullet(float realX, float realY, float lookX, float lookY);

	float getRealX() { return real_X; }
	float getRealY() { return real_Y; }
	
	bool overRange();

	void update(int frame_time);
};