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
	static const int ZIGZAG[64] =
	{
		0, 1, 8, 16, 9, 2, 3, 10,
		17, 24, 32, 25, 18, 11, 4, 5,
		12, 19, 26, 33, 40, 48, 41, 34,
		27, 20, 13, 6, 7, 14, 21, 28,
		35, 42, 49, 56, 57, 50, 43, 36,
		29, 22, 15, 23, 30, 37, 44, 51,
		58, 59, 52, 45, 38, 31, 39, 46,
		53, 60, 61, 54, 47, 55, 62, 63 };

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
		printf("Getting\n");
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
				fprintf(stderr, "%d Value not equal, original value = %x now value = %x\n", i + 1, data[i].val, s.val);
			}
		}
	}

	int JpegCoder::Encode(const ::Img<Rgb> &s, int level)
	{
		data.clear();
		::Img<Yuv> imgy = ImgRgb2YCbCr(s);
		ImgBlock<double> block = Img2Block(imgy);
		ImgBlock<double> dct = FDCT(block);
		ImgBlock<int> quant = Quant(dct, level);
		ZigZag<int>(quant);
		ImgBlockCode code = RunLengthCode(quant);
		BitStream imgStream;
		BuildHuffman(code);
		if (HuffmanEncode(code, imgStream))
		{
			fprintf(stderr, "[Error] Huffman encode error\n");
			return -1;
		}
		//Write
		EncodePicHead();
		EncodeQuantTable(level);
		EncodePicInfo(s.w, s.h);
		//Huffman table
		Huffman *huff[] = { &yDcHuff, &yAcHuff, &uvDcHuff, &uvAcHuff };
		bool isDc[] = { true, false, true, false };
		uint8_t htid[] = { 0, 0, 1, 1 };
		for (int tables = 0; tables < 4; ++tables)
		{
			const int *bitSize = huff[tables]->GetSize();
			const std::vector<int> &bitTable = huff[tables]->GetTable();
			EncodeHuffmanTable(bitSize, bitTable, isDc[tables], htid[tables]);
		}
		uint8_t *imgData = NULL;
		int imgLength = imgStream.GetData(&imgData);
		EncodeImg(imgData, imgLength);
		return 0;
	}

	int JpegCoder::Decode(Img<Rgb> &t)
	{
		unsigned idx = 0;
		int w = 0, h = 0;
		/*Handle file***********************************************************************************/
		bool isCommand = false; //Read 0xff
		uint8_t command = 0;
		bool finished = false;
		while (idx < data.size() && !finished)
		{
			while (data[idx] == 0xff)
			{
				//read 0xff, prepare for command
				isCommand = true;
				idx++;
			}
			if (!isCommand && data[idx] != 0xff)
			{
				fprintf(stderr, "[Error] Decode error, Unexpected byte 0x%u at %d\n", data[idx], idx);
				return -1;
			}
			command = data[idx];
			isCommand = false;
			//no length command
			if (command == 0xd8 || command == 0xd9)
			{
				idx++;
				continue;
				//start of image and end of image
			}

			if (idx + 2 >= data.size())
			{
				fprintf(stderr, "[Error] Read error, need more bytes than read\n");
				return -1;
			}
			int length = Read16(idx + 1); //Get segment length
			if (idx + 1 + length >= data.size())
			{ //Check data size
				fprintf(stderr, "[Error] Read error, need more bytes than read\n");
				return -1;
			}
			int returnVal = 0;
			switch (command)
			{
			case 0xe0:
				//picture head, not necessary for decode
				break;
			case 0xdb:
				//Quantization table
				returnVal = DecodeQuantTable(idx + 3, length - 2); //length should - 2bytes for length itself
				break;
			case 0xc0:
				returnVal = DecodePicInfo(idx + 3, length - 2, w, h);
				break;
			case 0xc4:
				//Huffman table
				returnVal = DecodeHuffmanTable(idx + 3, length - 2);
				break;
			case 0xda:
				//img table
				returnVal = DecodeImg(idx + 3, length - 2);
				finished = true;
				break;
			default:
				//other unnecessary code
				break;
			}
			if (returnVal != 0)
			{
				fprintf(stderr, "[Error] Decode error, exit\n");
				return -1;
			}
			idx += length + 1;
		}
		/*Decode*******************************************************************************/
		delete[] t.data;
		int wb = w / 8 + ((w % 8 == 0) ? 0 : 1);
		int hb = h / 8 + ((h % 8 == 0) ? 0 : 1);
		ImgBlockCode decode;
		decode.w = w;
		decode.h = h;
		decode.wb = wb;
		decode.hb = hb;
		decode.data = std::vector<BlockCode>(wb * hb);
		if (HuffmanDecode(decode))
		{
			fprintf(stderr, "[Error] Huffman decode error\n");
			return -1;
		}
		ImgBlock<int> dBlock = RunLengthDecode(decode);
		IZigZag<int>(dBlock);
		ImgBlock<double> dquant = Iquant(dBlock);
		ImgBlock<double> idct = FIDCT(dquant);
		Img<Yuv> iblock = Block2Img(idct);
		t = ImgYCbCr2Rgb(iblock);
		printf("[Decode] Finished\n");
		return 0;
	}

	int JpegCoder::Write(const char *fileName)
	{
		std::ofstream fd(fileName, std::ofstream::binary | std::ofstream::trunc);
		if (!fd)
		{
			fprintf(stderr, "[Error] Cannot open jpeg file\n");
			return -1;
		}
		fd.write((char *)(data.data()), data.size());
		fd.close();
		return 0;
	}

	int JpegCoder::Read(const char *fileName)
	{
		//Clear all
		data.clear();
		for (auto &t : qTable)
		{
			t.clear();
		}
		//Read data
		std::ifstream fd;
		fd.open(fileName, std::ifstream::binary);
		if (!fd)
		{
			fprintf(stderr, "[Error] Cannot read file %s\n", fileName);
			return -1;
		}
		fd.seekg(0, fd.end);
		int length = static_cast<int>(fd.tellg());
		uint8_t *buf = new uint8_t[length];
		fd.seekg(0, fd.beg);
		printf("[Reading] %d bytes\n", length);
		fd.read((char *)buf, length);
		if (!fd)
		{
			fprintf(stderr, "[Error] Read error\n");
			return -1;
		}
		data.insert(data.end(), buf, buf + length);
		delete[] buf;
		return 0;
	}

	void JpegCoder::EncodePicHead()
	{
		static const uint8_t headInfo[] = {
			0xff, 0xd8, 0xff, 0xe0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46, 0x00,
			0x01, 0x01, 0x01, 0x00, 0x60, 0x00, 0x60, 0x00, 0x00 };
		data.insert(data.end(), headInfo, headInfo + sizeof(headInfo));
	}

	void JpegCoder::EncodeQuantTable(int level)
	{
		/*The first table, for Y**************************/
		//write quantization table symbol
		uint8_t res;
		data.push_back(0xff);
		data.push_back(0xdb);
		//write table length
		uint16_t length = 67;
		Write16(length);
		//write table id, first table
		//low 4bit :id high 4bit precision
		data.push_back(0x00);
		int *yTable = GetYTable(level);
		//zigzag quant table
		for (int i = 0; i < 64; ++i)
		{
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
		int *uvTable = GetUvTable(level);
		//zigzag quant table
		for (int i = 0; i < 64; ++i)
		{
			res = static_cast<uint8_t>(uvTable[ZIGZAG[i]]);
			data.push_back(res);
		}
	}

	void JpegCoder::EncodePicInfo(uint16_t w, uint16_t h)
	{
		//Id
		data.push_back(0xff);
		data.push_back(0xc0);
		//length
		uint16_t length = 17;
		Write16(length);
		data.push_back(0x08); //precision
		Write16(h);			  //height
		Write16(w);			  //weight
		data.push_back(0x03); //number of component,3 component for YCbCr
		/*
			Each component 3 byte
			ID | Sample rate | Quant table id
			Component id:
			1: Y
			2: Cb
			3: Cr
			4: I
			5: Q
			*/
		uint8_t component[] = {
			0x01, 0x11, 0, //Y
			0x02, 0x11, 1, //Cb
			0x03, 0x11, 1  //Cr
		};
		for (int i = 0; i < sizeof(component); ++i)
		{
			data.push_back(component[i]);
		}
	}

	void JpegCoder::EncodeHuffmanTable(const int *bitSize, const std::vector<int> &bitTable, bool isDc, uint8_t htid)
	{
		data.push_back(0xff);
		data.push_back(0xc4);
		uint16_t length = static_cast<uint16_t>(2 + 1 + 16 + bitTable.size()); //length(2) + HT info(1) + Ref(16) + Val(huffman.length)
		Write16(length);
		//HT info
		uint8_t htInfo = 0;
		htInfo = htid & 0x0f;
		if (!isDc)
		{
			htInfo |= 0x10;
		}
		data.push_back(htInfo);
		for (int i = 0; i < 16; ++i)
		{
			data.push_back(static_cast<uint8_t>(bitSize[i]));
		}
		for (unsigned i = 0; i < bitTable.size(); ++i)
		{
			data.push_back(static_cast<uint8_t>(bitTable[i]));
		}
	}

	void JpegCoder::EncodeImg(uint8_t *s, int length)
	{
		//write header
		data.push_back(0xff);
		data.push_back(0xda);
		uint16_t headLength = 12;
		Write16(headLength);
		data.push_back(3); //component number
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
			3, 0x11 };
		data.insert(data.end(), componentTable, componentTable + sizeof(componentTable));
		//fixed value:Ss = 0x00, Se=63, Ah=Al=0x0(4bit)
		data.push_back(0);
		data.push_back(63);
		data.push_back(0);
		/*************************************************************************/
		//write image
		for (int i = 0; i < length; ++i)
		{
			data.push_back(s[i]);
			if (s[i] == 0xff)
				data.push_back(0x00);
		}
		//End of data
		data.push_back(0xff);
		data.push_back(0xd9);
	}

	int JpegCoder::DecodeQuantTable(int base, int length)
	{
		if (length < 65)
		{ //64 value and 1 QT info
			fprintf(stderr, "[Error] Bad quant table\n");
			return -1;
		}
		int quantNum = length / 64;
		for (int num = 0; num < quantNum; ++num)
		{
			uint8_t res;
			res = data[base + num * 65]; //qt info
			if (res >> 4 != 0)
			{
				fprintf(stderr, "[Error] Can only handle 8bit quantization table\n");
				return -1;
			}
			std::vector<int> &table = qTable[res & 0x0f]; //table id
			table.assign(64, 0);
			for (int i = 1; i <= 64; ++i)
			{
				table[ZIGZAG[i - 1]] = data[base + num * 65 + i];
			}
		}
		return 0;
	}

	int JpegCoder::DecodePicInfo(int base, int length, int &w, int &h)
	{
		if (length != 15)
		{
			fprintf(stderr, "[Error] Can only handle picture info segment with length 17\n");
			return -1;
		}
		//picture info
		/*
			Name		length	begin	end
			Precision	8		0		1
			Height		16		1		3
			Wight		16		3		5
			Components	8		5		6
			Each Component		Each Component
			ComponentId	8		6	9	12
			SampleRate	8		7	10	13
			QuantTable	8		8	11	14
			*/
		if (data[base] != 8 || data[base + 5] != 3)
		{
			fprintf(stderr, "[Error] Can only handle 8bit precision and 3 component\n");
			return -1;
		}
		h = Read16(base + 1);
		w = Read16(base + 3);
		uint8_t sample[3], quantTable[3];
		bool getYuv[3] = {};
		for (int i = 6; i <= 12; i += 3)
		{
			int id = data[base + i];
			if (id == 0 || id > 3)
			{
				fprintf(stderr, "[Error] Only support YCbCr\n");
				return -1;
			}
			sample[id - 1] = data[base + i + 1];
			quantTable[id - 1] = data[base + i + 2];
		}
		if (sample[0] != sample[1] || sample[1] != sample[2])
		{
			fprintf(stderr, "[Error] Down sample are not supported\n");
			return -1;
		}
		if (quantTable[1] != quantTable[2])
		{
			fprintf(stderr, "[Error] Cb and Cr only can share the same quantization table\n");
			return -1;
		}
		pYTable = &qTable[quantTable[0]];
		pUvTable = &qTable[quantTable[1]];
		return 0;
	}

	int JpegCoder::DecodeHuffmanTable(int base, int length)
	{
		if (length < 17)
		{
			fprintf(stderr, "[Error] Huffman table length error\n");
			return -1;
		}
		while (length > 17)
		{
			uint8_t ht = data[base];
			int id = ht & 0x0f;
			if (id > 3)
			{
				fprintf(stderr, "[Error] HT id should be 0-3\n");
				return -1;
			}
			bool isDc = true;
			if ((ht >> 4) & 1)
				isDc = false;
			if (ht >> 5 != 0)
			{
				fprintf(stderr, "[Warning] Check error, ht high 3 bit should be 0\n");
			}
			int numCode = 0;
			for (int i = 0; i < 16; ++i)
			{
				numCode += data[base + i + 1];
			}
			if (17 + numCode > length)
			{
				fprintf(stderr, "[Error] Huffman table error\n");
				return -1;
			}
			Huffman *pTable;
			if (isDc)
			{
				pTable = &dcHuff[id];
			}
			else
			{
				pTable = &acHuff[id];
			}
			pTable->SetTable(data.data() + base + 1, data.data() + base + 17, numCode);
			base += 17 + numCode;
			length -= 17 + numCode;
		}
		return 0;
	}

	int JpegCoder::DecodeImg(int base, int length)
	{
		if (length != 10)
		{
			fprintf(stderr, "[Error] Img segment length error\n");
			return -1;
		}
		int acHuffId[3] = {};
		int dcHuffId[3] = {};
		if (data[base] != 3)
		{
			fprintf(stderr, "[Error] Only 3 components are supported\n");
			return -1;
		}
		for (int i = 1; i <= 5; i += 2)
		{
			int id = data[base + i];
			if (id < 1 || id > 3)
			{
				fprintf(stderr, "[Error] Only support Y Cb Cr\n");
				return -1;
			}
			acHuffId[id - 1] = data[base + i + 1] & 0x0f;
			dcHuffId[id - 1] = data[base + i + 1] >> 4;
		}
		if (dcHuffId[1] != dcHuffId[2] || acHuffId[1] != acHuffId[2])
		{
			fprintf(stderr, "[Error] Cb and Cr should share the same huffman table, this software do not support two huffman tables\n");
			return -1;
		}
		if (data[base + 7] != 0x00 || data[base + 8] != 0x3f || data[base + 9] != 0x00)
		{
			fprintf(stderr, "[Warning] Img decode warning, check error\n");
		}
		yDcHuff = dcHuff[dcHuffId[0]];
		yAcHuff = acHuff[acHuffId[0]];
		uvDcHuff = dcHuff[dcHuffId[1]];
		uvAcHuff = acHuff[acHuffId[1]];
		base += 10;
		stream.clear();
		stream.push_back(BitStream{});
		BitStream *pImgStream = &stream[0];
		for (; (unsigned)base < data.size(); base++)
		{
			if (data[base] == 0xff)
			{
				if (data[base + 1] == 0)
				{
					pImgStream->PushBack(0xff);
					base++;
				}
				else if ((data[base + 1] & 0xf8) == 0xd0)
				{
					//RST
					base++;
					stream.push_back(BitStream{});
					pImgStream = &stream[stream.size() - 1];
				}
				else
				{
					break;
				}
			}
			else
			{
				pImgStream->PushBack(data[base]);
			}
		}
		return 0;
	}

	//Quant
	template <class T>
	ImgBlock<int> JpegCoder::Quant(T &block, int level)
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
	ImgBlock<double> JpegCoder::Iquant(T &block)
	{
		const std::vector<int> &yTable = *pYTable;
		const std::vector<int> &uvTable = *pUvTable;
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

	template <class T>
	void JpegCoder::ZigZag(ImgBlock<T> &block)
	{
		Block<T> res;
		for (int i = 0; i < block.wb * block.hb; ++i)
		{
			T *s[] = { block.data[i].y, block.data[i].u, block.data[i].v };
			T *t[] = { res.y, res.u, res.v };
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
	void JpegCoder::IZigZag(ImgBlock<T> &block)
	{
		Block<T> res;
		for (int i = 0; i < block.wb * block.hb; ++i)
		{
			T *s[] = { block.data[i].y, block.data[i].u, block.data[i].v };
			T *t[] = { res.y, res.u, res.v };
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

	ImgBlockCode JpegCoder::RunLengthCode(const ImgBlock<int> &block)
	{
		ImgBlockCode code;
		code.w = block.w;
		code.h = block.h;
		code.wb = block.wb;
		code.hb = block.hb;
		code.data = std::vector<BlockCode>(code.wb * code.hb);
		int lastYDc = 0, lastUDc = 0, lastVDc = 0;
		for (int i = 0; i < block.wb * block.hb; ++i)
		{
			//*****************************************************************
			//Get dc
			//*****************************************************************
			int diffY = block.data[i].y[0] - lastYDc;
			int diffU = block.data[i].u[0] - lastUDc;
			int diffV = block.data[i].v[0] - lastVDc;
			//code dc
			code.data[i].ydc = GetVLI(diffY);
			code.data[i].udc = GetVLI(diffU);
			code.data[i].vdc = GetVLI(diffV);
			lastYDc = block.data[i].y[0];
			lastUDc = block.data[i].u[0];
			lastVDc = block.data[i].v[0];
			//****************************************************************
			//Get Ac
			//****************************************************************
			const int *s[] = { block.data[i].y, block.data[i].u, block.data[i].v };
			std::vector<std::pair<uint8_t, Symbol>> *t[] = { &code.data[i].yac, &code.data[i].uac, &code.data[i].vac };
			for (int color = 0; color < 3; ++color)
			{
				int zeroNum = 0;
				auto ss = s[color];
				auto tt = t[color];
				//find eob
				int eob = 0;
				for (int i = 63; i >= 0; --i)
				{
					if (ss[i] != 0)
					{
						eob = i;
						break;
					}
				}
				for (int i = 1; i <= eob; ++i)
				{
					if (ss[i] == 0)
					{
						zeroNum++;
						if (zeroNum == 16)
						{
							zeroNum = 0;
							tt->emplace_back(0xf0, Symbol{ 0, 0 });
						}
					}
					else
					{
						Symbol val = GetVLI(ss[i]);
						uint8_t head = 0;
						head |= val.length & 0x0f;
						head |= (zeroNum & 0x0f) << 4;
						tt->emplace_back(head, val);
						zeroNum = 0;
					}
				}
				if (eob < 63)
				{
					tt->emplace_back(0x00, Symbol{ 0, 0 });
				}
			}
		}
		return code;
	}

	ImgBlock<int> JpegCoder::RunLengthDecode(const ImgBlockCode &code)
	{
		ImgBlock<int> block;
		block.w = code.w;
		block.h = code.h;
		block.wb = code.wb;
		block.hb = code.hb;
		int size = block.wb * block.hb;
		block.data = std::vector<Block<int>>(size);
		int lastYDc = 0;
		int lastUDc = 0;
		int lastVDc = 0;
		for (int i = 0; i < size; ++i)
		{
			if (code.data[i].restart) {
				lastYDc = 0;
				lastUDc = 0;
				lastVDc = 0;
			}
			lastYDc += DeVLI(code.data[i].ydc);
			lastUDc += DeVLI(code.data[i].udc);
			lastVDc += DeVLI(code.data[i].vdc);
			block.data[i].y[0] = lastYDc;
			block.data[i].u[0] = lastUDc;
			block.data[i].v[0] = lastVDc;
			int *t[] = { block.data[i].y, block.data[i].u, block.data[i].v };
			const std::vector<std::pair<uint8_t, Symbol>> *s[] = { &code.data[i].yac, &code.data[i].uac, &code.data[i].vac };
			for (int color = 0; color < 3; ++color)
			{
				int *tt = t[color];
				const std::vector<std::pair<uint8_t, Symbol>> &ss = *s[color];
				int idx = 1;
				for (auto &p : ss)
				{
					uint8_t head = p.first;
					Symbol data = p.second;
					if (head == 0x00)
					{ //EOB
						break;
					}
					int zeros = head >> 4;
					idx += zeros;
					if (idx >= 64)
					{
						fprintf(stderr, "[Error] RunLengthDecode error, try to get idx > 64\n");
						return block;
					}
					data.length = head & 0x0f;
					tt[idx] = DeVLI(data);
					idx++;
				}
			}
		}
		return block;
	}

	void JpegCoder::BuildHuffman(ImgBlockCode &block)
	{
		for (auto &b : block.data)
		{
			yDcHuff.Add(b.ydc.length);
			uvDcHuff.Add(b.udc.length);
			uvDcHuff.Add(b.vdc.length);
			for (auto &val : b.yac)
				yAcHuff.Add(val.first);
			for (auto &val : b.uac)
				uvAcHuff.Add(val.first);
			for (auto &val : b.vac)
				uvAcHuff.Add(val.first);
		}
		yDcHuff.BuildTable();
		yAcHuff.BuildTable();
		uvDcHuff.BuildTable();
		uvAcHuff.BuildTable();

		//print huffman table
		printf("L Dc table\n");
		yDcHuff.PrintTable();
		printf("L Ac table\n");
		yAcHuff.PrintTable();
		printf("C Dc table\n");
		uvDcHuff.PrintTable();
		printf("C Ac table\n");
		uvAcHuff.PrintTable();
	}

	int JpegCoder::HuffmanEncode(ImgBlockCode &block, BitStream &stream)
	{
		auto PrintErr = [](std::string msg = "") {
			fprintf(stderr, "[Error] Huffman Encode error %s\n", msg.data());
		};

		for (auto &b : block.data)
		{
			Symbol s;
			Symbol *dc[] = { &b.ydc, &b.udc, &b.vdc };
			std::vector<std::pair<uint8_t, Symbol>> *ac[] = { &b.yac, &b.uac, &b.vac };
			Huffman *dcCoder[] = { &yDcHuff, &uvDcHuff, &uvDcHuff };
			Huffman *acCoder[] = { &yAcHuff, &uvAcHuff, &uvAcHuff };
			for (int color = 0; color < 3; ++color)
			{
				if (dcCoder[color]->Encode(dc[color]->length, s))
				{
					PrintErr("Dc");
					return -1;
				}
				stream.Add(s);
				stream.Add(*dc[color]);
				for (auto &val : *ac[color])
				{
					if (acCoder[color]->Encode(val.first, s))
					{
						PrintErr("Ac");
						return -1;
					}
					stream.Add(s);
					stream.Add(val.second);
				}
			}
		}
		return 0;
	}

	int JpegCoder::HuffmanDecode(ImgBlockCode &code)
	{
		if (code.w == 0 || code.wb == 0 || code.h == 0 || code.hb == 0 || code.data.size() != code.hb * code.wb)
		{
			fprintf(stderr, "[Error] Huffman decode need image size\n");
			return -1;
		}
		int blockId = 0;
		std::vector<BitStream>::iterator pStream = stream.begin();
		while (blockId < code.wb * code.hb)
		{
			BlockCode &b = code.data[blockId];
			Symbol *dc[] = { &b.ydc, &b.udc, &b.vdc };
			std::vector<std::pair<uint8_t, Symbol>> *ac[] = { &b.yac, &b.uac, &b.vac };
			Huffman *dcCoder[] = { &yDcHuff, &uvDcHuff, &uvDcHuff };
			Huffman *acCoder[] = { &yAcHuff, &uvAcHuff, &uvAcHuff };
			for (int color = 0; color < 3; color++)
			{
				Symbol readData;
				readData.length = 0;
				readData.val = 0;
				Symbol tmp;
				tmp.length = 1;
				tmp.val = 0;
				uint8_t dcLength = 0;
				//Get Dc
				do
				{
					tmp.length = 1;
					if (pStream->Get(tmp) == -2) {
						//meet the end, change stream
						pStream++;
						if (pStream == stream.end()) {
							fprintf(stderr, "[Error] Want to read more than have\n");
							return -1;
						}
						b.restart = true;
						tmp.length = 1;
						tmp.val = 0;
						readData.length = 0;
						readData.val = 0;
						uint8_t acVal = 0;
						continue;
					}
					readData.length++;
					readData.val = readData.val << 1 | tmp.val;
					if (readData.length > 16)
					{
						fprintf(stderr, "[Error] Huffman decode error, dc decode error\n");
						return -1;
					}
				} while (dcCoder[color]->Decode(readData, dcLength));
				readData.length = dcLength;
				if (pStream->Get(readData) == -2) {
					//unexpected broken
					fprintf(stderr, "[Error] Unexpected broken\n");
					return -1;
				}
				*dc[color] = readData;
				//Get Ac
				int numValue = 1;
				while (numValue < 64)
				{
					readData.length = 0;
					readData.val = 0;
					uint8_t acVal = 0;
					do
					{
						tmp.length = 1;
						tmp.val = 0;
						if (pStream->Get(tmp) == -2) {
							//unexpected broken
							fprintf(stderr, "[Error] Unexpected broken\n");
							return -1;
						}
						readData.length++;
						readData.val = (readData.val << 1) | tmp.val & 1;
						if (readData.length > 16)
						{
							fprintf(stderr, "[Error] Huffman decode error, ac decode error\n");
							return -1;
						}
					} while (acCoder[color]->Decode(readData, acVal));
					if (acVal == 0xf0)
					{
						numValue += 16;
						ac[color]->emplace_back(acVal, Symbol{ 0, 0 });
						continue;
					}
					if (acVal == 0)
					{
						//EOB end of block
						ac[color]->emplace_back(0x00, Symbol{ 0, 0 });
						break;
					}
					int zeroNum = acVal >> 4;
					readData.length = acVal & 0x0f;
					if (pStream->Get(readData) == -2) {
						//unexpected broken
						fprintf(stderr, "[Error] Unexpected broken\n");
						return -1;
					}
					numValue += zeroNum + 1;
					ac[color]->emplace_back(acVal, readData);
				}
			}
			blockId++;
		}
		return 0;
	}

	Symbol JpegCoder::GetVLI(int val)
	{
		if (val == 0)
			return { 0, 0 };
		int middle = (val > 0) ? val : -val;
		Symbol ans;
		ans.length = 0;
		while (middle >> ans.length > 0)
		{
			ans.length++;
		}
		if (ans.length > 11)
		{
			fprintf(stderr, "[Warning] VLI value >= 2048\n");
		}
		if (val < 0)
			ans.val = (~middle) & ((1 << ans.length) - 1);
		else
			ans.val = middle;
		return ans;
	}

	int JpegCoder::DeVLI(Symbol s)
	{
		if (s.length == 0)
			return 0;
		if (s.val >> (s.length - 1) & 0x01)
		{
			return s.val;
		}
		int ans = ((~s.val) & ((1 << s.length) - 1));
		return -ans;
	}
}; //namespace jpeg