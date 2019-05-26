#include <cstdio>

#include "jpeg.h"

namespace jpeg {

	void test(const char* source, const char* target) {
		if (remove(target)) {
			fprintf(stderr, "[Warning] Delete failed\n");
		}
		Img<Rgb> img = bmp::ReadBmp(source);
		Img<Yuv>imgy = ImgRgb2YCbCr(img);

		Img<Yuv> dct = DCT(imgy);
		Img<Yuv> idct = IDCT(dct);

		Img<Rgb>imgrgb = ImgYCbCr2Rgb(idct);
		bmp::WriteBmp(target, imgrgb);
	}

};//namespace jpeg


