#pragma once
#ifndef __JPEG_H__
#define __JPEG_H__
#include <cmath>

#include "bmp.h"
#include "types.h"
#include "utilities.h"

namespace jpeg {

	void test(const char* source, const char* target);
	ImgChannel FDCT(Img<Yuv>);
	Img<Yuv> FIDCT(ImgChannel);
	ImgBlock Quant(ImgChannel &, int level = 0);
	ImgChannel Iquant(ImgBlock& block, int level = 0);
	ImgBlock ZigZag(const ImgBlock&);
	ImgBlock IZigZag(const ImgBlock&);

	int * GetYTable(int level);
	int * GetUvTable(int level);

	class BitStream {
	public:
		BitStream() :data(), remain(0) {};
		void Add(const Symbol& s);

	private:
		std::vector<uint8_t> data;
		int remain = 0;
		
	};//namespace jpeg

};//namespace jpeg

#endif/*__JPEG_H__*/