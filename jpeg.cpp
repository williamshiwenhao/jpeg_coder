#include <cstdio>
#include <random>
#include <algorithm>
#include <string>

#include "jpeg.h"
#include "huffman.h"

namespace jpeg {


	void test(const char* source, const char* target) {
		if (remove(target)) {
			fprintf(stderr, "[Warning] Delete failed\n");
		}
		Img<Rgb> img = bmp::ReadBmp(source);
		Img<Yuv>imgy = ImgRgb2YCbCr(img);
		ImgBlock<double> block = Img2Block(imgy);
		ImgBlock<double> dct = FDCT(block);
		ImgBlock<int> quant = Quant(dct);
		ZigZag<int>(quant);
		ImgBlockCode code = RunLengthCode(quant);
		//HuffmanEncode(code);

		ImgBlock<int> decode = RunLengthDecode(code);
		IZigZag<int>(decode);
		ImgBlock<double> dquant = Iquant(decode);
		ImgBlock<double> idct = FIDCT(dquant);
		Img<Yuv> iblock = Block2Img(idct);

		Img<Rgb>imgrgb = ImgYCbCr2Rgb(iblock);
		bmp::WriteBmp(target, imgrgb);
	}

	//Quant
	template<class T>
	ImgBlock<int> Quant(T& block, int level) {
		int* yTable = GetYTable(level);
		int* uvTable = GetUvTable(level);
		ImgBlock<int> ans;
		ans.w = block.w;
		ans.h = block.h;
		ans.wb = block.wb;
		ans.hb = block.hb;
		ans.data = std::vector<Block<int>>(ans.wb*ans.hb);
		for (int i = 0; i < ans.wb * ans.hb; ++i) {
			for (int j = 0; j < 64; ++j) {
				ans.data[i].y[j] = static_cast<int>(block.data[i].y[j] / yTable[j]);
				ans.data[i].u[j] = static_cast<int>(block.data[i].u[j] / uvTable[j]);
				ans.data[i].v[j] = static_cast<int>(block.data[i].v[j] / uvTable[j]);
			}
		}
		return ans;
	}

	template<class T>
	ImgBlock<double> Iquant(T& block, int level) {
		int* yTable = GetYTable(level);
		int* uvTable = GetUvTable(level);
		ImgBlock<double> ans;
		ans.w = block.w;
		ans.h = block.h;
		ans.wb = block.wb;
		ans.hb = block.hb;
		ans.data = std::vector<Block<double>>(ans.wb*ans.hb);
		for (int i = 0; i < ans.wb * ans.hb; ++i) {
			for (int j = 0; j < 64; ++j) {
				ans.data[i].y[j] = block.data[i].y[j] * yTable[j];
				ans.data[i].u[j] = block.data[i].u[j] * uvTable[j];
				ans.data[i].v[j] = block.data[i].v[j] * uvTable[j];
			}
		}
		return ans;
	}

	//ZigZag
	static const int ZIGZAG[64] =
	{
		 0,  1,  8, 16,  9,  2,  3, 10,
		17, 24, 32, 25, 18, 11,  4,  5,
		12, 19, 26, 33, 40, 48, 41, 34,
		27, 20, 13,  6,  7, 14, 21, 28,
		35, 42, 49, 56, 57, 50, 43, 36,
		29, 22, 15, 23, 30, 37, 44, 51,
		58, 59, 52, 45, 38, 31, 39, 46,
		53, 60, 61, 54, 47, 55, 62, 63,
	};
	template<class T>
	void ZigZag(ImgBlock<T>& block) {
		Block<T> res;
		for (int i = 0; i < block.wb * block.hb; ++i) {
			T *s[] = { block.data[i].y, block.data[i].u, block.data[i].v };
			T *t[] = { res.y, res.u, res.v };
			for (int color = 0; color < 3; ++color) {
				for (int j = 0; j < 64; ++j) {
					t[color][ZIGZAG[j]] = s[color][j];
				}
				for (int j = 0; j < 64; ++j) {
					s[color][j] = t[color][j];
				}
			}
		}
	}

	template<class T>
	void IZigZag(ImgBlock<T>& block) {
		Block<T> res;
		for (int i = 0; i < block.wb * block.hb; ++i) {
			T *s[] = { block.data[i].y, block.data[i].u, block.data[i].v };
			T *t[] = { res.y, res.u, res.v };
			for (int color = 0; color < 3; ++color) {
				for (int j = 0; j < 64; ++j) {
					t[color][j] = s[color][ZIGZAG[j]];
				}
				for (int j = 0; j < 64; ++j) {
					s[color][j] = t[color][j];
				}
			}
		}
	}

	static const uint8_t ones[9] = {0,0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f,0xff};
	void BitStream::Add(const Symbol & s)
	{
		int unfinished = s.length;
		if (unfinished <= 0)
			return;
		if (remain != 0) {
			uint8_t tail = data.back();
			int diff = s.length - remain;
			if (diff >= 0) {
				uint8_t add= (s.val >> diff) & ones[remain];
				tail = tail & (~ones[remain]) | add;
				data[data.size() - 1] = tail;
				remain = 0;
				unfinished = diff;
			}
			else {
				uint8_t add = (s.val << (-diff)) & ones[remain];
				tail = tail & (~ones[remain]) | add;
				data[data.size() - 1] = tail;
				remain = -diff;
				return;
			}
		}
		while (unfinished > 8) {
			unfinished -= 8;
			data.emplace_back(static_cast<uint8_t>((s.val >> unfinished) & ones[8]));
		}
		if (unfinished != 0) {
			remain = 8 - unfinished;
			data.emplace_back(static_cast<uint8_t>((s.val << remain) & ones[8]));
		}
		else {
			remain = 0;	
		}
	}

	int BitStream::print()
	{
		for (auto & i : data) {
			printf("%x ", i);
		}
		printf("\n");
		return data.size() * 8 - remain;
	}

};//namespace jpeg


