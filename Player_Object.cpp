#include "Surpport.h"
#include "Player_Object.h"

Player::Player(int code, float init_X, float init_Y, float init_HP) {
	real_X = init_X;		real_Y = init_Y;
	hp = init_HP;			id = code;
	move_State[0] = move_State[1] = 0;
	shoot_State = NO_SHOOT;
	shoot_CollTime = 0.3f;
	kill = death = 0;
	coll_Box[LEFT] = init_X - PLAYER_SIZE;
	coll_Box[RIGHT] = init_X + PLAYER_SIZE;
	coll_Box[BOTTOM] = init_Y - PLAYER_SIZE;
	coll_Box[TOP] = init_Y + PLAYER_SIZE;
}

float Player::move(float min, float pos, float veloc, float max) {
	pos += veloc;
	if (pos < min)	return min;
	if (pos > max)	return max;
	return pos;
}

bool Player::shoot() {
	if (shoot_State == SHOOT && shoot_CollTime < 0.0f) {
		shoot_CollTime = 0.3f;
		return true;
	}
	return false;
}

bool Player::collBullet(float damage) {
	hp -= damage;
	if (hp <= 0)
		return true;
	return false;
}

void Player::changeMove(int* moveState) {
	move_State[0] = moveState[0];
	move_State[1] = moveState[1];
}

void Player::changeShootState(int state) {
	shoot_State = state;
}

void Player::changeLookXY(float lookX, float lookY) {
	look_X = lookX;		look_Y = lookY;
}

void Player::respawn(float pX, float pY, float pHP) {
	real_X = pX;		real_Y = pY;
	hp = pHP;
	move_State[0] = 0;	move_State[1] = 0;
	shoot_State = NO_SHOOT;
	shoot_CollTime = 0.3f;
	coll_Box[LEFT] = pX - PLAYER_SIZE;
	coll_Box[RIGHT] = pX + PLAYER_SIZE;
	coll_Box[BOTTOM] = pY - PLAYER_SIZE;
	coll_Box[TOP] = pY + PLAYER_SIZE;
}

void Player::eatItem() {
	hp += RECOVERY;
	if (hp > PLAYER_HP)
		hp = PLAYER_HP;
}

void Player::reset(float pX, float pY, float pHP) {
	real_X = pX;		real_Y = pY;
	hp = pHP;
	move_State[0] = 0;	move_State[1] = 0;
	shoot_State = NO_SHOOT;
	shoot_CollTime = 0.3f;
	kill = death = 0;
	coll_Box[LEFT] = pX - PLAYER_SIZE;
	coll_Box[RIGHT] = pX + PLAYER_SIZE;
	coll_Box[BOTTOM] = pY - PLAYER_SIZE;
	coll_Box[TOP] = pY + PLAYER_SIZE;
}

void Player::update(int frame_time) {
	shoot_CollTime -= ((float)frame_time / 1000.0f);
	
	float v_PPS = (PLAYER_SPEED * KPH_TO_MPS * METTER_TO_PIXEL);
	float v_PPMS = v_PPS * ((float)frame_time / 1000.0f);
	if ((move_State[0] * move_State[1]) != 0)
		v_PPMS /= 1.414f;
	
	real_X = move(PLAYER_SIZE, real_X, move_State[0] * v_PPMS, 2000.0f - PLAYER_SIZE);
	real_Y = move(PLAYER_SIZE, real_Y, move_State[1] * v_PPMS, 2000.0f - PLAYER_SIZE);
	
	coll_Box[LEFT] = real_X - PLAYER_SIZE;
	coll_Box[RIGHT] = real_X + PLAYER_SIZE;
	coll_Box[BOTTOM] = real_Y - PLAYER_SIZE;
	coll_Box[TOP] = real_Y + PLAYER_SIZE;
}