#include "Surpport.h"
#include "Bullet_Object.h"

Bullet::Bullet(
	int init_owner,
	float init_X, float init_Y,
	float p_LookX, float p_LookY) 
{

	owner = init_owner;
	start_X = init_X;	start_Y = init_Y;
	real_X = init_X;		real_Y = init_Y;
	vector_X = p_LookX;	vector_Y = p_LookY;

	coll_Box[LEFT] = init_X - (BULLET_SIZE * 1.5f);
	coll_Box[RIGHT] = init_X + (BULLET_SIZE * 1.5f);
	coll_Box[BOTTOM] = init_Y - (BULLET_SIZE * 1.5f);
	coll_Box[TOP] = init_Y + (BULLET_SIZE * 1.5f);
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
	
	coll_Box[LEFT] = real_X - (BULLET_SIZE * 1.5f);
	coll_Box[RIGHT] = real_X + (BULLET_SIZE * 1.5f);
	coll_Box[BOTTOM] = real_Y - (BULLET_SIZE * 1.5f);
	coll_Box[TOP] = real_Y + (BULLET_SIZE * 1.5f);
}