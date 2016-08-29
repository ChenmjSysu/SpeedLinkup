#ifndef COMMON
#define COMMON

#include <string>
using std::string;
struct Point {
    int x;
	int y;
	Point(int x_ = 0, int y_ = 0): x(x_), y(y_) {}
	~Point() {}

	string ToString() {
		char* str = new char[100];
		sprintf(str, "(%d, %d)", x, y);
		return string(str);
	}
};

#endif