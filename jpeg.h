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

	class JpegWriter {
	public:
		int Encode(const Img<Rgb>& s, int level);
		int Write(const char* fileName);

	private:
		inline void Write16(uint16_t val) {
			uint8_t res;
			res = (val >> 8)& 0xff;
			data.push_back(res);
			res = val & 0xff;
			data.push_back(res);
		}
		void PicHead();
		void QuantTable(int level = 0);
		void PicInfo(uint16_t w, uint16_t);
		void HuffmanTable(const int* bitSize, const std::vector<int>& bitTable, bool isDc, uint8_t htid);
		void Img(uint8_t *s, int length);
		std::vector<uint8_t> data;
	};

};//namespace jpeg

#endif/*__JPEG_H__*/