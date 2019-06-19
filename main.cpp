#include <cstdio>
#include <string>
#include <fstream>
#include <cmath>

#include "bmp.h"
#include "types.h"
#include "jpeg.h"

using namespace std;

char encodeInput[] = "test2.bmp";
char encodeOutput[] = "test2.jpg";
char decodeInput[] = "PsResult.jpg";
char decodeOutput[] = "test1Decode.bmp";

void Encode(const char* name, int level = 0) {
	Img<Rgb> img = bmp::ReadBmp(encodeInput);
	jpeg::JpegCoder writer;
	writer.Encode(img, level);
	writer.Write(name);
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
	string encodeNameBase{ "encode" };
	for (int i = 0; i < 4; ++i) {
		//level test
		string name = encodeNameBase + to_string(i) + ".jpg";
		Encode(name.c_str(), i);
	}
	Decode();
	return 0;
}