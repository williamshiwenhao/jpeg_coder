#include <cstdio>
#include <string>
#include <fstream>
#include <cmath>

#include "bmp.h"
#include "types.h"
#include "jpeg.h"

using namespace std;

string sourceFile = "test1.bmp";
string jpegFile = "result1.jpeg";
string bmpFile = "result1.bmp";
bool CheckStatified(double* coded, int* column, int* row, size_t codedSize, size_t HSize);
int main()
{
	jpeg::test(sourceFile.c_str(), bmpFile.c_str());
	return 0;
}
