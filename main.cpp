#include <cstdio>
#include <string>
#include <fstream>
#include <cmath>

#include "bmp.h"
#include "types.h"
#include "jpeg.h"

using namespace std;

char encodeInput[] = "test1.bmp";
char encodeOutput[] = "test1.jpg";
char decodeInput[] = "PsResult.jpg";
char decodeOutput[] = "test1Decode.bmp";

void Encode() {
	Img<Rgb> img = bmp::ReadBmp(encodeInput);
	jpeg::JpegCoder writer;
	writer.Encode(img, 0);
	writer.Write(encodeOutput);
}

void Decode() {
	Img<Rgb> decode;
	jpeg::JpegCoder reader;

	reader.Read(decodeInput);
	reader.Decode(decode);
	bmp::WriteBmp(decodeOutput, decode);
}

int main()
{
	//Encode();
	Decode();
	return 0;
}
