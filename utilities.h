#pragma once
#ifndef __UTILITIES_H__
#define __UTILITIES_H__
#include "types.h"
Img<Yuv> ImgRgb2YCbCr(const Img<Rgb> );
Img<Rgb> ImgYCbCr2Rgb(const Img<Yuv>);

inline Yuv Rgb2YCbCr(const Rgb);
inline Rgb YCbCr2Rgb(const Yuv);
template<class T> inline uint8_t ColorCast(const T&);
ImgBlock<double> Img2Block(Img<Yuv> img);
Img<Yuv> Block2Img(ImgBlock<double>& block);

#endif/*__UTILITIES_H__*/