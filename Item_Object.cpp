#include "Item_Object.h"
#include "Surpport.h"

Item::Item(float realX, float realY) {
	real_X = realX;		real_Y = realY;
	coll_Box[0] = real_X - ITEM_SIZE;
	coll_Box[1] = real_X + ITEM_SIZE;
	coll_Box[2] = real_Y - ITEM_SIZE;
	coll_Box[3] = real_Y + ITEM_SIZE;
	respawn_time = 0;
	state = ITEM_EXIST;
}

void Item::eating() {
	state = ITEM_NONE;
}

bool Item::respawnOK() {
	return respawn_time >= ITEM_RESPAWN_TIME;
}

void Item::setRespawn(float realX, float realY) {
	real_X = realX;		real_Y = realY;
	coll_Box[0] = real_X - ITEM_SIZE;
	coll_Box[1] = real_X + ITEM_SIZE;
	coll_Box[2] = real_Y - ITEM_SIZE;
	coll_Box[3] = real_Y + ITEM_SIZE;
	respawn_time = 0;
	state = ITEM_EXIST;
}

void Item::update(int frame_time) {
	if (state == ITEM_NONE) {
		respawn_time += frame_time;
	}
}