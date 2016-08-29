#include "Common.h";

class Block {
public:
	Block() {};
	Block(unsigned short label, Point pos, Point click): Label(label), Position(pos), ClickPosition(click) {};
	unsigned short Label;
	Point Position;
	Point ClickPosition;
};