#include <cstdio>
#include <string>
#include <fstream>
#include <cmath>

#include "bmp.h"
#include "types.h"

using namespace std;

string sourceFile = "test.bmp";
string jpegFile = "C:\\Users\\william\\Pictures\\test.jpeg";
string bmpFile = "test1.bmp";
bool CheckStatified(double* coded, int* column, int* row, size_t codedSize, size_t HSize);
int main()
{
	printf("Hello world\n");
	Img<Rgb> img = bmp::ReadBmp(sourceFile.c_str());
	bmp::WriteBmp(bmpFile.c_str(), img);
	delete[] img.data;
	return 0;
}