#pragma once
#ifndef __JPEG_H__
#define __JPEG_H__
#include <cmath>

#include "bmp.h"
#include "types.h"
#include "utilities.h"

namespace jpeg {

	void test(const char* source, const char* target);
	ImgChannel FDCT(Img<Yuv>);
	Img<Yuv> FIDCT(ImgChannel&);

};//namespace jpeg

#endif/*__JPEG_H__*/