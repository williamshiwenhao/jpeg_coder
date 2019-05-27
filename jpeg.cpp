#include <cstdio>
#include <random>

#include "jpeg.h"

namespace jpeg {

	void watchDct(ImgChannel & ch) {
		int wb = ch.w / 8;
		int hb = ch.h / 8;
		std::random_device rd;
		int xb = rd() % wb;
		int yb = rd() % hb;
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
	}

	void test(const char* source, const char* target) {
		if (remove(target)) {
			fprintf(stderr, "[Warning] Delete failed\n");
		}
		Img<Rgb> img = bmp::ReadBmp(source);
		Img<Yuv>imgy = ImgRgb2YCbCr(img);

		ImgChannel dct = FDCT(imgy);
		Img<Yuv> idct = FIDCT(dct);
		//watchDct(dct);

		Img<Rgb>imgrgb = ImgYCbCr2Rgb(idct);
		bmp::WriteBmp(target, imgrgb);
	}

};//namespace jpeg


