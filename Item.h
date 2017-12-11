#pragma once
#include <time.h>

class Item {
	float real_X, real_Y;
	float coll_Box[4];
	clock_t currentTime;
public:
	Item(float realX, float realY);
	float const getRealX() { return real_X; }
	float const getRealY() { return real_Y; }
	float* const getCollBox() { return coll_Box; }
	clock_t clockCurrentTIme() { return currentTime = clock(); }
	clock_t getCurrentTime() { return currentTime; }
	void setCurrentTime(int time);
};