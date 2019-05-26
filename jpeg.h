#pragma once
#ifndef __JPEG_H__
#define __JPEG_H__
#include <cmath>

#include "bmp.h"
#include "types.h"
#include "utilities.h"

namespace jpeg {

	void test(const char* source, const char* target);
	Img<Yuv> DCT(Img<Yuv>);
	Img<Yuv> IDCT(Img<Yuv>);

};//namespace jpeg

#endif/*__JPEG_H__*/