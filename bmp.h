#ifndef __BMP_H__
#define __BMP_H__

#include <atlimage.h>
#include <cstdio>

#include "types.h"

namespace bmp {
	Img<Rgb> ReadBmp(const char* fileName);
	int WriteBmp(const char* fileName, Img<Rgb>);
};//namespace bmp

#endif /*__BMP_H__*/