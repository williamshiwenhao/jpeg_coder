#include <cstdio>
#include <random>
#include <algorithm>
#include <string>

#include "jpeg.h"

namespace jpeg {

	void watchChannel(ImgChannel & ch, int num = 1) {
		int wb = ch.w / 8;
		int hb = ch.h / 8;
		std::random_device rd;
		int xb = rd() % wb;
		int yb = rd() % hb;
		for (int x = 0; x < num; ++x) {
			printf("xb = %d yb = %d\n", xb, yb);
			printf("Y block:\n");
			for (int x = xb * 8; x < xb * 8 + 8; ++x) {
				for (int y = yb * 8; y < yb * 8 + 8; ++y) {
					int idx = y * ch.w + x;
					printf("%f ", ch.y[idx]);
				}
				printf("\n");
			}
			printf("Cb block:\n");
			for (int x = xb * 8; x < xb * 8 + 8; ++x) {
				for (int y = yb * 8; y < yb * 8 + 8; ++y) {
					int idx = y * ch.w + x;
					printf("%f ", ch.cb[idx]);
				}
				printf("\n");
			}
			printf("Cr block:\n");
			for (int x = xb * 8; x < xb * 8 + 8; ++x) {
				for (int y = yb * 8; y < yb * 8 + 8; ++y) {
					int idx = y * ch.w + x;
					printf("%f ", ch.cr[idx]);
				}
				printf("\n");
			}
			std::string name[] = { "Y", "Cb", "Cr" };
			std::vector<double> * ptr[] = { &ch.y, &ch.cb, &ch.cr };
			for (int color = 0; color < 3; ++color) {
				double minNum = *std::min_element(ptr[color]->begin(), ptr[color]->end());
				double maxNum = *std::max_element(ptr[color]->begin(), ptr[color]->end());
				printf("%s max element = %lf min element = %lf\n", name[color].c_str(), maxNum, minNum);
			}
			xb = (xb + 1) % wb;
		}
	}

	void test(const char* source, const char* target) {
		if (remove(target)) {
			fprintf(stderr, "[Warning] Delete failed\n");
		}
		Img<Rgb> img = bmp::ReadBmp(source);
		Img<Yuv>imgy = ImgRgb2YCbCr(img);
		ImgChannel ch = Img2Channel(imgy);
		//watchChannel(ch);
		ImgChannel dct = FDCT(imgy);
		ImgBlock quant = Quant(dct);
		ImgBlock zigzag = ZigZag(quant);


		ImgBlock izigzag = IZigZag(zigzag);
		ImgChannel dquant = Iquant(izigzag);
		Img<Yuv> idct = FIDCT(dquant);
		watchChannel(dquant, 3);

		Img<Rgb>imgrgb = ImgYCbCr2Rgb(idct);
		bmp::WriteBmp(target, imgrgb);
	}

	//Quant
	ImgBlock Quant(ImgChannel& ch, int level) {
		int wb = static_cast<int>(ceil(double(ch.w) / 8.0));
		int hb = static_cast<int>(ceil(double(ch.h) / 8.0));
		ImgBlock block;
		block.w = ch.w;
		block.h = ch.h;
		block.wb = wb;
		block.hb = hb;
		block.data = std::vector<Block>(wb * hb);
		int * yQuantTable = GetYTable(level);
		int * uvQuantTable = GetUvTable(level);
		for (int xb = 0; xb < wb; ++xb) {
			for (int yb = 0; yb < hb; ++yb) {
				for (int i = 0; i < 64; ++i) {
					int x = xb * 8 + i % 8;
					int y = yb * 8 + i / 8;
					if (x >= ch.w || y >= ch.h)
						continue;
					int idx = y * ch.w + x;
					int blockId = yb * wb + xb;
					block.data[blockId].y[i] = static_cast<int>(ch.y[idx] / yQuantTable[i]);
					block.data[blockId].u[i] = static_cast<int>(ch.cb[idx] / uvQuantTable[i]);
					block.data[blockId].v[i] = static_cast<int>(ch.cr[idx] / uvQuantTable[i]);
				}
			}
		}
		return block;
	}

	ImgChannel Iquant(ImgBlock& block, int level) {
		ImgChannel ch;
		ch.w = block.w;
		ch.h = block.h;
		int &wb = block.wb;
		int &hb = block.hb;
		ch.y = std::vector<double>(ch.w * ch.h);
		ch.cb = std::vector<double>(ch.w * ch.h);
		ch.cr = std::vector<double>(ch.w * ch.h);
		int * yQuantTable = GetYTable(level);
		int * uvQuantTable = GetUvTable(level);
		for (int xb = 0; xb < wb; ++xb) {
			for (int yb = 0; yb < hb; ++yb) {
				for (int i = 0; i < 64; ++i) {
					int x = xb * 8 + i % 8;
					int y = yb * 8 + i / 8;
					if (x >= ch.w || y >= ch.h)
						continue;
					int idx = y * ch.w + x;
					int blockId = yb * wb + xb;
					ch.y[idx] = block.data[blockId].y[i] * yQuantTable[i];
					ch.cb[idx] = block.data[blockId].u[i] * uvQuantTable[i];
					ch.cr[idx] = block.data[blockId].v[i] * uvQuantTable[i];
				}
			}
		}
		return ch;
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
	ImgBlock ZigZag(const ImgBlock& block) {
		ImgBlock ans = block;
		for (int i = 0; i < block.wb * block.hb; ++i) {
			const int *s[]= { block.data[i].y, block.data[i].u, block.data[i].v };
			int *t[] = { ans.data[i].y, ans.data[i].u, ans.data[i].v };
			for (int color = 0; color < 3; ++color) {
				for (int j = 0; j < 64; ++j) {
					t[color][ZIGZAG[j]] = s[color][j];
				}
			}
		}
		return ans;
	}

	ImgBlock IZigZag(const ImgBlock& block) {
		ImgBlock ans = block;
		for (int i = 0; i < block.wb * block.hb; ++i) {
			const int *s[] = { block.data[i].y, block.data[i].u, block.data[i].v };
			int *t[] = { ans.data[i].y, ans.data[i].u, ans.data[i].v };
			for (int color = 0; color < 3; ++color) {
				for (int j = 0; j < 64; ++j) {
					t[color][j] = s[color][ZIGZAG[j]];
				}
			}
		}
		return ans;
	}

	static const uint8_t ones[9] = {0,0x01,0x03,0x07,0x0f,0x1f,0x3f,0x7f,0xff};
	void BitStream::Add(const Symbol & s)
	{
		int unfinished = s.length;
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
				unfinished = 0;
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

};//namespace jpeg


