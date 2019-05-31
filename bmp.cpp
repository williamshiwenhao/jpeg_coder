#include <cstring>
#include <iostream>
#include <atlimage.h>

#include "bmp.h"
#include "types.h"

namespace bmp {
	Img<Rgb> ReadBmp(const char* fileName) {
		CImage img;
		Img<Rgb> rgbImg;
		if (img.Load(fileName))
		{
			fprintf(stderr, "[BMP Error] Cannot load bmp picture\n");
			return rgbImg;
		}
		printf("[Read] Succeed\n");
		int bitCount = img.GetBPP();
		int w = img.GetWidth();
		int h = img.GetHeight();
		printf("[Info] BitCount=%d Width=%d Height=%d\n",bitCount, w, h);
		if (bitCount != 24)
		{
			fprintf(stderr, "[BMP Error] Can only handle 24bit bitmap\n");
			return rgbImg;
		}

		Rgb* data = new Rgb[w*h];
		for (int x = 0; x < w; ++x)
		{
			for (int y = 0; y < h; ++y)
			{
				int idx = y * w + x;
				COLORREF color = img.GetPixel(x, y);
				Rgb &target = data[idx];
				uint8_t * cColor = (uint8_t*)&color;
				target.r = cColor[0];
				target.g = cColor[1];
				target.b = cColor[2];
			}
		}
		rgbImg.data = data;
		rgbImg.w = w;
		rgbImg.h = h;
		return rgbImg;
	}

	int WriteBmp(const char* fileName, Img<Rgb> rgbImg)
	{
		CImage img;
		if (img.Create(rgbImg.w, rgbImg.h, 24)==0) {
			//set bitcount to default 24
			fprintf(stderr, "[Error] CImage create error\n");
			return -1;
		}
		for (int x = 0; x < rgbImg.w; ++x)
		{
			for (int y = 0; y < rgbImg.h; ++y) {
				int idx = y * rgbImg.w + x;
				Rgb& color = rgbImg.data[idx];
				img.SetPixel(x, y, RGB(color.r, color.g, color.b) );
			}
		}	
		HRESULT result = img.Save(fileName);
		if (result != 0) {
			fprintf(stderr, "[Error] Save bmp error, error number: %d\n", result);
		}
		return 0;
	}
};//namespace bmp