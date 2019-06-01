#ifndef __TYPE_H__
#define __TYPE_H__

#include <cstdint>
#include <vector>

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


template<class T>
struct Block {
	T y[64] = {};
	T u[64] = {};
	T v[64] = {};
};

template<class T>
struct ImgBlock {
	int w, h, wb, hb;
	std::vector<Block<T>> data;
};

struct Symbol {
	uint32_t val;
	int length;
	bool operator<(const Symbol& s)const { return val < s.val; }
};

struct BlockCode {
	Symbol ydc,udc,vdc;
	std::vector<std::pair<uint8_t, Symbol>> yac, uac, vac;//<zeroNumber|size, number in vli>
};

struct ImgBlockCode {
	int w, h, wb, hb;
	std::vector<BlockCode> data;
};

#endif // !__TYPE_H__
