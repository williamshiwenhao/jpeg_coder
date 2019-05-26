#pragma once
#ifndef __UTILITIES_H__
#define __UTILITIES_H__
#include "types.h"
Img<Yuv> ImgRgb2YCbCr(const Img<Rgb> );
Img<Rgb> ImgYCbCr2Rgb(const Img<Yuv>);

inline Yuv Rgb2YCbCr(const Rgb);
inline Rgb YCbCr2Rgb(const Yuv);
inline uint8_t ColorCast(const double);

#endif/*__UTILITIES_H__*/