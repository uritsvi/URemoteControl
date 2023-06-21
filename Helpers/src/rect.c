#include "rect.h"

bool _collide(
	Rect* rect1, 
	Rect* rect2) {
	
	if (rect1->left <= rect2->right && 
		rect1->right >= rect2->left && 
		rect1->top <= rect2->bottom &&
		rect1->bottom >= rect2->top) {

		return true;
	}

	return false;
}

bool collide(
	Rect* rect1, 
	Rect* rect2) {
	
	bool res = _collide(rect1, rect2);
	res |= _collide(rect2, rect1);

	return res;




}
