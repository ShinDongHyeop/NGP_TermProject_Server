#include "Item.h"
#include "suf.h"

Item::Item(float realX, float realY) {
	real_X = realX;		real_Y = realY;
	currentTime = 0;
	coll_Box[LEFT] = realX - ITEM_SIZE;
	coll_Box[RIGHT] = realX + ITEM_SIZE;
	coll_Box[BOTTOM] = realY - ITEM_SIZE;
	coll_Box[TOP] = realY + ITEM_SIZE;
}

void Item::setCurrentTime(int time)
{
	currentTime = time;
}