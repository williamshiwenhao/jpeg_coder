#include <cstdio>
#include <random>
#include <algorithm>
#include <string>

#include "jpeg.h"
#include "huffman.h"
#include "utilities.h"

namespace jpeg
{
void BitStreamTest()
{
	std::vector<int> ones;
	for (int i = 0; i <= 16; ++i)
	{
		ones.push_back((1 << i) - 1);
	}
	BitStream stream;
	const int TESTSIZE = 1 << 16;
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<int> length(1, 16);
	std::uniform_int_distribution<int> value(0, (1 << 16) - 1);
	std::vector<Symbol> data;
	printf("Adding\n");
	for (int i = 0; i < TESTSIZE; ++i)
	{
		Symbol s;
		s.length = length(gen);
		s.val = value(gen) & ones[s.length];
		data.push_back(s);
		stream.Add(s);
	}
	printf("Geting\n");
	for (int i = 0; i < TESTSIZE; ++i)
	{
		Symbol s;
		s.length = data[i].length;
		if (stream.Get(s))
		{
			fprintf(stderr, "Get error\n");
		}
		if (s.val != data[i].val)
		{
			fprintf(stderr, "%d Value not equel, original value = %x now value = %x\n", i + 1, data[i].val, s.val);
		}
	}
}

void findDiff(ImgBlockCode& s, ImgBlockCode& t) {
	for (int i = 0; i < s.wb * s.hb; ++i) {
		BlockCode &bs = s.data[i];
		Symbol *dcs[] = { &bs.ydc, &bs.udc, &bs.vdc };
		std::vector<std::pair<uint8_t, Symbol>> *acs[] = { &bs.yac, &bs.uac, &bs.vac };
		BlockCode &bt = t.data[i];
		Symbol *dct[] = { &bt.ydc, &bt.udc, &bt.vdc };
		std::vector<std::pair<uint8_t, Symbol>> *act[] = { &bt.yac, &bt.uac, &bt.vac };
		for (int color = 0; color < 3; ++color) {
			//check dc
			if (dcs[color]->val != dct[color]->val) {
				printf("Dc error, block %d, color %d\n", i, color);
			}
			if (acs[color]->size() != acs[color]->size()) {
				printf("Ac size not eque\n");
			}
			else {
				for (int j = 0; j < acs[color]->size(); ++j) {
					if ((*acs[color])[j].first != (*act[color])[j].first) {
						printf("Ac value error\n");
					}
				}
			}
		}
	}
}

void test(const char *source, const char *target)
{
	printf("Bit stream test\n");
	BitStreamTest();
	printf("Bit stream test finished\n");
	if (remove(target))
	{
		fprintf(stderr, "[Warning] Delete failed\n");
	}
	Img<Rgb> img = bmp::ReadBmp(source);
	Img<Yuv> imgy = ImgRgb2YCbCr(img);
	ImgBlock<double> block = Img2Block(imgy);
	ImgBlock<double> dct = FDCT(block);
	ImgBlock<int> quant = Quant(dct);
	ZigZag<int>(quant);
	ImgBlockCode code = RunLengthCode(quant);
	BitStream imgStream;
	Huffman yDcHuff, yAcHuff, uvDcHuff, uvAcHuff;
	BuildHuffman(code, yDcHuff, yAcHuff, uvDcHuff, uvAcHuff);
	if (HuffmanEncode(code, imgStream, yDcHuff, yAcHuff, uvDcHuff, uvAcHuff))
	{
		fprintf(stderr, "[Error] Huffman encode error\n");
	}

	//Decode
	ImgBlockCode decode;
	decode.w = code.w;
	decode.h = code.h;
	decode.wb = code.wb;
	decode.hb = code.hb;
	decode.data = std::vector<BlockCode>(decode.wb * decode.hb);
	if (HuffmanDecode(imgStream, decode, yDcHuff, yAcHuff, uvDcHuff, uvAcHuff))
	{
		fprintf(stderr, "[Error] Huffman decode error\n");
	}
	//findDiff(code, decode);
	ImgBlock<int> deBlock = RunLengthDecode(decode);
	IZigZag<int>(deBlock);
	ImgBlock<double> dquant = Iquant(deBlock);
	ImgBlock<double> idct = FIDCT(dquant);
	Img<Yuv> iblock = Block2Img(idct);

	Img<Rgb> imgrgb = ImgYCbCr2Rgb(iblock);
	bmp::WriteBmp(target, imgrgb);
}

//Quant
template <class T>
ImgBlock<int> Quant(T &block, int level)
{
	int *yTable = GetYTable(level);
	int *uvTable = GetUvTable(level);
	ImgBlock<int> ans;
	ans.w = block.w;
	ans.h = block.h;
	ans.wb = block.wb;
	ans.hb = block.hb;
	ans.data = std::vector<Block<int>>(ans.wb * ans.hb);
	for (int i = 0; i < ans.wb * ans.hb; ++i)
	{
		for (int j = 0; j < 64; ++j)
		{
			ans.data[i].y[j] = static_cast<int>(block.data[i].y[j] / yTable[j]);
			ans.data[i].u[j] = static_cast<int>(block.data[i].u[j] / uvTable[j]);
			ans.data[i].v[j] = static_cast<int>(block.data[i].v[j] / uvTable[j]);
		}
	}
	return ans;
}

template <class T>
ImgBlock<double> Iquant(T &block, int level)
{
	int *yTable = GetYTable(level);
	int *uvTable = GetUvTable(level);
	ImgBlock<double> ans;
	ans.w = block.w;
	ans.h = block.h;
	ans.wb = block.wb;
	ans.hb = block.hb;
	ans.data = std::vector<Block<double>>(ans.wb * ans.hb);
	for (int i = 0; i < ans.wb * ans.hb; ++i)
	{
		for (int j = 0; j < 64; ++j)
		{
			ans.data[i].y[j] = block.data[i].y[j] * yTable[j];
			ans.data[i].u[j] = block.data[i].u[j] * uvTable[j];
			ans.data[i].v[j] = block.data[i].v[j] * uvTable[j];
		}
	}
	return ans;
}

//ZigZag
static const int ZIGZAG[64] =
	{
		0, 1, 8, 16, 9, 2, 3, 10,
		17, 24, 32, 25, 18, 11, 4, 5,
		12, 19, 26, 33, 40, 48, 41, 34,
		27, 20, 13, 6, 7, 14, 21, 28,
		35, 42, 49, 56, 57, 50, 43, 36,
		29, 22, 15, 23, 30, 37, 44, 51,
		58, 59, 52, 45, 38, 31, 39, 46,
		53, 60, 61, 54, 47, 55, 62, 63};
template <class T>
void ZigZag(ImgBlock<T> &block)
{
	Block<T> res;
	for (int i = 0; i < block.wb * block.hb; ++i)
	{
		T *s[] = {block.data[i].y, block.data[i].u, block.data[i].v};
		T *t[] = {res.y, res.u, res.v};
		for (int color = 0; color < 3; ++color)
		{
			for (int j = 0; j < 64; ++j)
			{
				t[color][ZIGZAG[j]] = s[color][j];
			}
			for (int j = 0; j < 64; ++j)
			{
				s[color][j] = t[color][j];
			}
		}
	}
}

template <class T>
void IZigZag(ImgBlock<T> &block)
{
	Block<T> res;
	for (int i = 0; i < block.wb * block.hb; ++i)
	{
		T *s[] = {block.data[i].y, block.data[i].u, block.data[i].v};
		T *t[] = {res.y, res.u, res.v};
		for (int color = 0; color < 3; ++color)
		{
			for (int j = 0; j < 64; ++j)
			{
				t[color][j] = s[color][ZIGZAG[j]];
			}
			for (int j = 0; j < 64; ++j)
			{
				s[color][j] = t[color][j];
			}
		}
	}
}

void BitStream::Add(const Symbol &s)
{
	if (s.length > 16 || s.length < 0)
	{
		fprintf(stderr, "[Error] Bitstream add length %d\n", s.length);
		return;
	}
	int remainBit = 8 - tail;
	if (remainBit > s.length)
	{
		uint8_t &res = data.back();
		SetBitByte(res, tail, s.length, s.val);
		tail += s.length;
	}
	else
	{
		int sPos = 0;
		if (remainBit > 0)
		{
			uint8_t res = GetBitSymbol(s, sPos, remainBit);
			uint8_t &last = data.back();
			SetBitByte(last, tail, remainBit, res);
			sPos += remainBit;
		}
		tail = 8;
		while (s.length - sPos > 8)
		{
			uint8_t res = GetBitSymbol(s, sPos, 8);
			data.push_back(res);
			sPos += 8;
		}
		if (s.length - sPos > 0)
		{
			tail = s.length - sPos;
			uint8_t res = GetBitSymbol(s, sPos, tail);
			uint8_t last = 0;
			SetBitByte(last, 0, tail, res);
			data.push_back(last);
		}
	}
}

int BitStream::Get(Symbol &s)
{
	if (s.length > 16 || s.length < 0)
	{
		fprintf(stderr, "[Error] Bitstream get length %d\n", s.length);
		return -1;
	}
	s.val = 0;
	int remainBit = 8 - head;
	if (remainBit > s.length)
	{
		s.val = GetBitByte(data[readIdx], head, s.length);
		head += s.length;
	}
	else
	{
		int sPos = 0;
		uint8_t res = 0;
		if (remainBit > 0)
		{
			res = GetBitByte(data[readIdx], head, remainBit);
			SetBitSymbol(s, sPos, remainBit, res);
			sPos += remainBit;
		}
		readIdx++;
		head = 0;
		while (s.length - sPos >= 8)
		{
			SetBitSymbol(s, sPos, 8, data[readIdx++]);
			sPos += 8;
			if (readIdx >= data.size())
			{
				fprintf(stderr, "[Error] Get bit more than have\n");
				return -1;
			}
		}
		if (s.length - sPos > 0)
		{
			head = s.length - sPos;
			res = GetBitByte(data[readIdx], 0, head);
			SetBitSymbol(s, sPos, head, res);
		}
	}
	return 0;
}

int BitStream::print()
{
	for (auto &i : data)
	{
		printf("%x ", i);
	}
	printf("\n");
	return data.size() * 8 - tail;
}

int BitStream::GetData(uint8_t **t)
{
	if (t != NULL)
		*t = data.data();
	return data.size();
}

void BitStream::SetData(std::vector<uint8_t>::iterator begin, std::vector<uint8_t>::iterator end)
{
	data = std::vector<uint8_t>(begin, end);
	tail = 8;
	head = 0;
	readIdx = 0;
}

}; //namespace jpeg
