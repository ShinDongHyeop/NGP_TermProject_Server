#include "Bullet.h"
#include "suf.h"

Bullet::Bullet(float pOwner, float realX, float realY, float lookX, float lookY) {
	owner = pOwner;
	start_X = realX;	start_Y = realY;
	real_X = realX;		real_Y = realY;
	vector_X = lookX;	vector_Y = lookY;
	range = BULLET_RANGE;
	coll_Box[LEFT] = realX - BULLET_SIZE;
	coll_Box[RIGHT] = realX + BULLET_SIZE;
	coll_Box[BOTTOM] = realY - BULLET_SIZE;
	coll_Box[TOP] = realY + BULLET_SIZE;
}

bool Bullet::overRange() {
	float num = sqrt(
		((start_X - real_X) * (start_X - real_X)) +
		((start_Y - real_Y) * (start_Y - real_Y))
	);
	return (num > BULLET_RANGE);
}

void Bullet::update(int frame_time) {
	float v_PPS = (BULLET_SPEED * KPH_TO_MPS * METTER_TO_PIXEL);
	float v_PPMS = v_PPS * ((float)frame_time / 1000.0f);
	real_X += (vector_X * v_PPMS);
	real_Y += (vector_Y * v_PPMS);
	coll_Box[LEFT] = real_X - BULLET_SIZE;
	coll_Box[RIGHT] = real_X + BULLET_SIZE;
	coll_Box[BOTTOM] = real_Y - BULLET_SIZE;
	coll_Box[TOP] = real_Y + BULLET_SIZE;
}