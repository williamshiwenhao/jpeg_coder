#include <cstdio>
#include <string>
#include <fstream>
#include <cmath>

#include "bmp.h"
#include "types.h"
#include "jpeg.h"

using namespace std;

char sourceFile[] = "test1.bmp";
char jpegFile[] = "result1.jpg";
char bmpFile[] = "result2.bmp";
bool CheckStatified(double* coded, int* column, int* row, size_t codedSize, size_t HSize);
int main()
{
	Img<Rgb> img = bmp::ReadBmp(sourceFile);
	jpeg::JpegWriter writer;
	writer.Encode(img, 0);
	writer.Write(jpegFile);

	return 0;
}
