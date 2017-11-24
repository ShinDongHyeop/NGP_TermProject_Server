#include <stdio.h>
#include "Player.h"
#include "suf.h"

Player::Player(float pX, float pY, float pHP) {
	real_X = pX;		real_Y = pY;
	hp = pHP;
	speed = PLAYER_SPEED;
	move_State[0] = 0;	move_State[1] = 0;
	shoot_State = NO_SHOOT;
	shoot_CollTime = 0.3f;
}

bool Player::shoot() {
	if (shoot_State == SHOOT && shoot_CollTime < 0.0f) {
		shoot_CollTime = 0.3f;
		return true;
	}
	return false;
}

float Player::move(float min, float x, float v, float max) {
	x += v;
	if (x < min)	return min;
	if (x > max)	return max;
	return x;
}

void Player::changeMove(int* moveState) {
	move_State[0] = moveState[0];
	move_State[1] = moveState[1];
}

void Player::update(int frame_time) {
	shoot_CollTime -= ((float)frame_time / 1000.0f);
	float v_PPS = (PLAYER_SPEED * KPH_TO_MPS * METTER_TO_PIXEL);
	float v_PPMS = v_PPS * ((float)frame_time / 1000.0f);
	if ((move_State[0] * move_State[1]) != 0)
		v_PPMS /= 1.414;
	real_X = move(PLAYER_SIZE, real_X, move_State[0] * v_PPMS, 2000.0f - PLAYER_SIZE);
	real_Y = move(PLAYER_SIZE, real_Y, move_State[1] * v_PPMS, 2000.0f - PLAYER_SIZE);
}

void Player::changeShootState(int state) {
	shoot_State = state;
}

void Player::changeLookXY(float lookX, float lookY) {
	look_X = lookX;		look_Y = lookY;
}