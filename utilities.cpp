
#include "utilities.h"

Img<Yuv> ImgRgb2YCbCr(const Img<Rgb> imgrgb) {
	Img<Yuv> imgy;
	imgy.w = imgrgb.w;
	imgy.h = imgrgb.h;
	imgy.data = new Yuv[imgy.w * imgy.h];
	for (int i = 0; i < imgy.w * imgy.h; ++i) {
		imgy.data[i] = Rgb2YCbCr(imgrgb.data[i]);
	}
	return imgy;
}

Img<Rgb> ImgYCbCr2Rgb(const Img<Yuv> imgy) {
	Img<Rgb> imgrgb;
	imgrgb.w = imgy.w;
	imgrgb.h = imgy.h;
	imgrgb.data = new Rgb[imgy.w * imgy.h];
	for (int i = 0; i < imgy.w * imgy.h; ++i) {
		imgrgb.data[i] = YCbCr2Rgb(imgy.data[i]);
	}
	return imgrgb;
}

inline Yuv Rgb2YCbCr(const Rgb rgb) {
	Yuv y;
	y.y = ColorCast(0.299 * rgb.r + 0.587 * rgb.g + 0.114 * rgb.b);
	y.cb = ColorCast(-0.168 * rgb.r - 0.3313 * rgb.g + 0.5 * rgb.b + 128);
	y.cr = ColorCast(0.5 * rgb.r - 0.4187 * rgb.g - 0.0813 * rgb.b + 128);
	return y;
}
inline Rgb YCbCr2Rgb(const Yuv y) {
	Rgb r;
	r.r = ColorCast(y.y + 1.402 * (y.cr - 128));
	r.g = ColorCast(y.y - 0.34414 * (y.cb - 128) - 0.71414 * (y.cr - 128));
	r.b = ColorCast(y.y + 1.772 * (y.cb - 128));
	return r;
}

template<class T>
inline uint8_t ColorCast(const T& dc) {
	if (dc < 0)
		return 0;
	if (dc > 255)
		return 255;
	return static_cast<uint8_t>(dc);
}

static const double dctBase = 128;
ImgBlock<double> Img2Block(Img<Yuv> img) {
	int wb = (img.w >> 3) + ((img.w % 8 >0)?1 : 0);
	int hb = (img.h >> 3) + ((img.h % 8 > 0) ? 1 : 0);
	ImgBlock<double> block;
	block.w = img.w;
	block.h = img.h;
	block.wb = wb;
	block.hb = hb;
	block.data = std::vector<Block<double>>(wb * hb);
	for (int i = 0; i < img.w * img.h; ++i) {
		int x = i % img.w;
		int y = i / img.w;
		int xb = x / 8;
		int yb = y / 8;
		int blkId = yb * wb + xb;
		int xIn = x % 8;
		int yIn = y % 8;
 		int inId = yIn * 8 + xIn;
		block.data[blkId].y[inId] = img.data[i].y - dctBase;
		block.data[blkId].u[inId] = img.data[i].cb - dctBase;
		block.data[blkId].v[inId] = img.data[i].cr - dctBase;
	}
	return block;
}

Img<Yuv> Block2Img(ImgBlock<double>& block) {
	int &wb = block.wb;
	int &hb = block.hb;
	Img<Yuv> img;
	img.w = block.w;
	img.h = block.h;
	img.data = new Yuv[img.w * img.h];
	for (int i = 0; i < img.w * img.h; ++i) {
		int x = i % img.w;
		int y = i / img.w;
		int xb = x / 8;
		int yb = y / 8;
		int blkId = yb * wb + xb;
		int xIn = x % 8;
		int yIn = y % 8;
		int inId = yIn * 8 + xIn;
		img.data[i].y = ColorCast(block.data[blkId].y[inId] + dctBase);
		img.data[i].cb = ColorCast(block.data[blkId].u[inId] + dctBase);
		img.data[i].cr = ColorCast(block.data[blkId].v[inId] + dctBase);
	}
	return img;
}


