#pragma once
#ifndef __JPEG_H__
#define __JPEG_H__
#include <cmath>
#include <unordered_map>
#include <queue>

#include "bmp.h"
#include "types.h"
#include "utilities.h"

namespace jpeg {

	void test(const char* source, const char* target);
	ImgBlock<double> FDCT(ImgBlock<double>);
	ImgBlock<double> FIDCT(ImgBlock<double>);
	template<class T> ImgBlock<int> Quant(T& block, int level=0);
	template<class T> ImgBlock<double> Iquant(T& block, int level=0);
	template<class T> void ZigZag(ImgBlock<T>& block);
	template<class T> void IZigZag(ImgBlock<T>& block);

	int * GetYTable(int level);
	int * GetUvTable(int level);

	class BitStream {
	public:
		void Add(const Symbol& s);
		int Get(Symbol& s);
		int print();//for debug
		int GetData(uint8_t**);
		void SetData(std::vector<uint8_t>::iterator begin, std::vector<uint8_t>::iterator end);
	private:
		std::vector<uint8_t> data ;
		int tail = 8;
		int head = 0;
		unsigned int readIdx = 0;
		
	};

};//namespace jpeg

#endif/*__JPEG_H__*/