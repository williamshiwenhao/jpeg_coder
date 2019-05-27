
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

inline uint8_t ColorCast(const double dc) {
	if (dc < 0)
		return 0;
	if (dc > 255)
		return 255;
	return static_cast<uint8_t>(dc);
}

ImgChannel Img2Channel(Img<Yuv> img) {
	ImgChannel ans;
	ans.w = img.w;
	ans.h = img.h;
	int imgSize = ans.w * ans.h;
	ans.y.assign(ans.w*ans.h, 0);
	ans.cb.assign(ans.w*ans.h, 0);
	ans.cr.assign(ans.w*ans.h, 0);
	for (int i = 0; i < imgSize; ++i) {
		ans.y[i] = img.data[i].y - 128;
		ans.cb[i] = img.data[i].cb - 128;
		ans.cr[i] = img.data[i].cr - 128;
	}
	return ans;
}

Img<Yuv> Channel2Img(ImgChannel ch) {
	Img<Yuv> img;
	img.w = ch.w;
	img.h = ch.h;
	int imgSize = ch.w * ch.h;
	img.data = new Yuv[imgSize];
	for (int i = 0; i < imgSize; ++i) {
		img.data[i].y = ColorCast(ch.y[i] + 128);
		img.data[i].cb = ColorCast(ch.cb[i] + 128);
		img.data[i].cr = ColorCast(ch.cr[i] + 128);
	}
	return;
}