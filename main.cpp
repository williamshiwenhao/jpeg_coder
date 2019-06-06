#include <cstdio>
#include <string>
#include <fstream>
#include <cmath>

#include "bmp.h"
#include "types.h"
#include "jpeg.h"

using namespace std;

char sourceFile[] = "test2.bmp";
char jpegFile[] = "result2.jpg";
char bmpFile[] = "result2.bmp";
bool CheckStatified(double* coded, int* column, int* row, size_t codedSize, size_t HSize);
int main()
{
	Img<Rgb> img = bmp::ReadBmp(sourceFile);
	jpeg::JpegCoder writer;
	writer.Encode(img, 0);
	writer.Write(jpegFile);

	return 0;
}
