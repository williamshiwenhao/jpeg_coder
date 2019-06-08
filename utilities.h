#pragma once
#ifndef __UTILITIES_H__
#define __UTILITIES_H__
#include "types.h"
//Change image color
Img<Yuv> ImgRgb2YCbCr(const Img<Rgb>);
Img<Rgb> ImgYCbCr2Rgb(const Img<Yuv>);
//Change color
inline Yuv Rgb2YCbCr(const Rgb);
inline Rgb YCbCr2Rgb(const Yuv);
//double to uint8_t, handle value > 255 and < 0
template<class T> inline uint8_t ColorCast(const T&);

ImgBlock<double> Img2Block(Img<Yuv> img);
Img<Yuv> Block2Img(ImgBlock<double>& block);
//For bit stream
inline uint8_t GetBitSymbol(const Symbol &s, int pos, int length)
{
	uint8_t ans = 0;
	if (length < 0 || length > 8 || pos + length > s.length) {
		fprintf(stderr, "[Error] Get Bit error, length error\n");
		return ans;
	}
	ans = s.val >> (s.length - pos - length);
	ans &= ones[length];
	return ans;
}

inline void SetBitSymbol(Symbol &s, int pos, int length, uint8_t value) {
	int base = s.length - pos - length;
	uint32_t mask = ones[length] << base;
	uint32_t res = value;
	res = (res << base) & mask;
	s.val = (s.val & ~mask) | res;
}

inline uint8_t GetBitByte(const uint8_t s, int pos, int length) {
	uint8_t ans = 0;
	if (length < 0 || length > 8 || pos + length > 8) {
		fprintf(stderr, "[Error] GetBit byte error, length error\n");
		return ans;
	}
	int base = 8 - pos - length;
	ans = (s >> base) & ones[length];
	return ans;
}

inline void SetBitByte(uint8_t &s, int pos, int length, uint8_t value) {
	if (length < 0 || length > 8 || pos + length > 8) {
		fprintf(stderr, "[Error] SetBit byte error, length error\n");
		return;
	}
	int base = 8 - pos - length;
	uint8_t mask = ones[length] << base;
	uint8_t res = (value << base) & mask;
	s = (s & ~mask) | res;
}

#endif/*__UTILITIES_H__*/