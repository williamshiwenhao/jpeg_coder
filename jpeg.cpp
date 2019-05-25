#include "jpeg.h"

void test(const char* source, const char* target) {
	Img<Rgb> img = bmp::ReadBmp(source);
	Img<YCbCr>imgy = ImgRgb2YCbCr(img);
	Img<Rgb>imgrgb = ImgYCbCr2Rgb(imgy);
	bmp::WriteBmp(target, imgrgb);
}