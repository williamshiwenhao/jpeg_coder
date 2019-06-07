#include <cstdio>
#include <string>
#include <fstream>
#include <cmath>

#include "bmp.h"
#include "types.h"
#include "jpeg.h"

using namespace std;

char sourceFile[] = "test1.bmp";
char jpegFile[] = "PsResult.jpg";
char bmpFile[] = "resultPs.bmp";
int main()
{
	//Img<Rgb> img = bmp::ReadBmp(sourceFile);
	Img<Rgb> decode;
	jpeg::JpegCoder writer,reader;
	//writer.Encode(img, 0);
	//writer.Write(jpegFile);
	reader.Read(jpegFile);
	reader.Decode(decode);
	bmp::WriteBmp(bmpFile, decode);
	return 0;
}
