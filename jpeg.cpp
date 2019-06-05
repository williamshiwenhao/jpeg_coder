#include <cstdio>
#include <random>
#include <algorithm>
#include <string>
#include <fstream>

#include "jpeg.h"
#include "huffman.h"
#include "utilities.h"
#include "types.h"

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
				t[color][j] = s[color][ZIGZAG[j]];
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
				t[color][ZIGZAG[j]] = s[color][j];
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

int JpegWriter::Encode(const ::Img<Rgb>& s, int level)
{
	data.clear();
	::Img<Yuv> imgy = ImgRgb2YCbCr(s);
	ImgBlock<double> block = Img2Block(imgy);
	ImgBlock<double> dct = FDCT(block);
	ImgBlock<int> quant = Quant(dct,level);
	ZigZag<int>(quant);
	ImgBlockCode code = RunLengthCode(quant);
	BitStream imgStream;
	Huffman yDcHuff, yAcHuff, uvDcHuff, uvAcHuff;
	BuildHuffman(code, yDcHuff, yAcHuff, uvDcHuff, uvAcHuff);
	if (HuffmanEncode(code, imgStream, yDcHuff, yAcHuff, uvDcHuff, uvAcHuff))
	{
		fprintf(stderr, "[Error] Huffman encode error\n");
		return -1;
	}
	//Write 
	PicHead();
	QuantTable(level);
	PicInfo(s.w, s.h);
	//huffman table
	Huffman *huff[] = { &yDcHuff, &yAcHuff, &uvDcHuff, &uvAcHuff };
	bool isDc[] = { true , false, true, false};
	uint8_t htid[] = { 0 , 0 , 1, 1 };
	for (int tables = 0; tables < 4; ++tables) {
		const int* bitSize = huff[tables]->GetSize();
		const std::vector<int>& bitTable = huff[tables]->GetTable();
		HuffmanTable(bitSize, bitTable, isDc[tables], htid[tables]);
	}
	uint8_t *imgData = NULL;
	int imgLength = imgStream.GetData(&imgData);
	Img(imgData, imgLength);
	return 0;
}

int JpegWriter::Write(const char * fileName)
{
	std::ofstream fd(fileName, std::ofstream::binary | std::ofstream::trunc);
	if (!fd) {
		fprintf(stderr, "[Error] Cannot open jpeg file\n");
		return -1;
	}
	fd.write((char*)(data.data()), data.size());
	fd.close();
	return 0;
}

void JpegWriter::PicHead()
{
	static const uint8_t headInfo[] = {
		0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46, 0x00,
		0x01, 0x01, 0x01, 0x00, 0x60, 0x00, 0x60, 0x00, 0x00
	};
	data.insert(data.end(), headInfo, headInfo + sizeof(headInfo));
}

void JpegWriter::QuantTable(int level)
{
	/*The first table, for Y**************************/
	//write quanttable symbol
	uint8_t res;
	data.push_back(0xff);
	data.push_back(0xdb);
	//write table length
	uint16_t length = 67;
	Write16(length);
	//write table id, first table
	//low 4bit :id high 4bit precision
	data.push_back(0x00);
	int* yTable = GetYTable(level);
	//zigzag quant table
	for (int i = 0; i < 64; ++i) {
		res = static_cast<uint8_t>(yTable[ZIGZAG[i]]);
		data.push_back(res);
	}
	/*The second table, for C**************************/
	//write quanttable symbol
	data.push_back(0xff);
	data.push_back(0xdb);
	//write table length
	Write16(length);
	//write table id, second table
	//low 4bit :id high 4bit precision
	data.push_back(0x01);
	int* uvTable = GetUvTable(level);
	//zigzag quant table
	for (int i = 0; i < 64; ++i) {
		res = static_cast<uint8_t>(uvTable[ZIGZAG[i]]);
		data.push_back(res);
	}
}

void JpegWriter::PicInfo(uint16_t w, uint16_t h)
{
	//Id
	data.push_back(0xff);
	data.push_back(0xc0);
	//length
	uint16_t length = 17;
	Write16(length);
	data.push_back(0x08);//precision
	Write16(h);//heigth
	Write16(w);//weigth
	data.push_back(0x03);//number of component,3 commonent for YCbCr
	/*
	Each component 3 byte
	ID | Sample rate | Quant table id
	Compoent id:
	1: Y
	2: Cb
	3: Cr
	4: I
	5: Q
	*/
	uint8_t component[] = {
		0x01, 0x11, 0,//Y
		0x02, 0x11, 1,//Cb
		0x03, 0x11, 1 //Cr
	};
	for (int i = 0; i < sizeof(component); ++i) {
		data.push_back(component[i]);
	}
}

void JpegWriter::HuffmanTable(const int* bitSize, const std::vector<int>& bitTable, bool isDc, uint8_t htid)
{
	data.push_back(0xff);
	data.push_back(0xc4);
	uint16_t length = static_cast<uint16_t>(2+1+16+bitTable.size());//length(2) + HT info(1) + Ref(16) + Val(huffman.length)
	Write16(length);
	//HT info
	uint8_t htInfo = 0;
	htInfo = htid & 0x0f;
	if (!isDc) {
		htInfo |= 0x10;
	}
	data.push_back(htInfo);
	for (int i = 0; i < 16; ++i) {
		data.push_back(static_cast<uint8_t>(bitSize[i]));
	}
	for (int i = 0; i < bitTable.size(); ++i) {
		data.push_back(static_cast<uint8_t>(bitTable[i]));
	}
}

void JpegWriter::Img(uint8_t * s, int length)
{
	//write header
	data.push_back(0xff);
	data.push_back(0xda);
	uint16_t headLength = 12;
	Write16(headLength);
	data.push_back(3);//component number
	/*
	Each have its id and huffman table id;
	Component id start from 1
	table id:
		high 4bit: DC table
		low 4bit : Ac table
	*/
	uint8_t componentTable[] = {
		1, 0x00,
		2, 0x11,
		3, 0x11
	};
	data.insert(data.end(), componentTable, componentTable + sizeof(componentTable));
	//fixed value:Ss = 0x00, Se=63, Ah=Al=0x0(4bit)
	data.push_back(0);
	data.push_back(63);
	data.push_back(0);
	/*************************************************************************/
	//write img
	for (int i = 0; i < length; ++i) {
		data.push_back(s[i]);
		if (s[i] == 0xff)
			data.push_back(0x00);
	}
	//End of data
	data.push_back(0xff);
	data.push_back(0xd9);
}

}; //namespace jpeg
