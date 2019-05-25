#pragma once
#ifndef __UTILITIES_H__
#define __UTILITIES_H__
#include "types.h"
Img<YCbCr> ImgRgb2YCbCr(const Img<Rgb> );
Img<Rgb> ImgYCbCr2Rgb(const Img<YCbCr>);

inline YCbCr Rgb2YCbCr(const Rgb);
inline Rgb YCbCr2Rgb(const YCbCr);

#endif/*__UTILITIES_H__*/