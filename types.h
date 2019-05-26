#ifndef __TYPE_H__
#define __TYPE_H__

#include <cstdint>

struct Rgb {
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

struct Yuv {
	uint8_t y;
	uint8_t cb;
	uint8_t cr;
};

template<class Color>
struct Img {
	int w, h;
	Color *data;
	Img() :w(0), h(0), data(NULL) {}
};

#endif // !__TYPE_H__
