#include "huffman.h"

#include <queue>
#include <string>
#include <algorithm>

namespace jpeg
{
ImgBlockCode RunLengthCode(const ImgBlock<int> &block)
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
		/*get dc**********************************************************/
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
		/*get ac*********************************************************/
		const int *s[] = {block.data[i].y, block.data[i].u, block.data[i].v};
		std::vector<std::pair<uint8_t, Symbol>> *t[] = {&code.data[i].yac, &code.data[i].uac, &code.data[i].vac};
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
						tt->emplace_back(0xf0, Symbol{0, 0});
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
				tt->emplace_back(0x00, Symbol{0, 0});
			}
		}
	}
	return code;
}

ImgBlock<int> RunLengthDecode(const ImgBlockCode &code)
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
		lastYDc += DeVLI(code.data[i].ydc);
		lastUDc += DeVLI(code.data[i].udc);
		lastVDc += DeVLI(code.data[i].vdc);
		block.data[i].y[0] = lastYDc;
		block.data[i].u[0] = lastUDc;
		block.data[i].v[0] = lastVDc;
		int *t[] = {block.data[i].y, block.data[i].u, block.data[i].v};
		const std::vector<std::pair<uint8_t, Symbol>> *s[] = {&code.data[i].yac, &code.data[i].uac, &code.data[i].vac};
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

void BuildHuffman(ImgBlockCode &block, Huffman &yDcHuff, Huffman &yAcHuff, Huffman &uvDcHuff, Huffman &uvAcHuff)
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

int HuffmanEncode(ImgBlockCode &block, BitStream &stream, Huffman &yDcHuff, Huffman &yAcHuff, Huffman &uvDcHuff, Huffman &uvAcHuff)
{

	auto PrintErr = [](std::string msg = "") {
		fprintf(stderr, "[Error] Huffman Encode error %s\n", msg.data());
	};

	for (auto &b : block.data)
	{
		Symbol s;
		Symbol *dc[] = {&b.ydc, &b.udc, &b.vdc};
		std::vector<std::pair<uint8_t, Symbol>> *ac[] = {&b.yac, &b.uac, &b.vac};
		Huffman *dcCoder[] = {&yDcHuff, &uvDcHuff, &uvDcHuff};
		Huffman *acCoder[] = {&yAcHuff, &uvAcHuff, &uvAcHuff};
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

int HuffmanDecode(BitStream &s, ImgBlockCode &code, Huffman &yDcHuff, Huffman &yAcHuff, Huffman &uvDcHuff, Huffman &uvAcHuff)
{
	if (code.w == 0 || code.wb == 0 || code.h == 0 || code.hb == 0 || code.data.size() != code.hb * code.wb)
	{
		fprintf(stderr, "[Error] Huffman decode need img size\n");
		return -1;
	}
	int blockId = 0;
	while (blockId < code.wb * code.hb)
	{
		BlockCode &b = code.data[blockId];
		Symbol *dc[] = {&b.ydc, &b.udc, &b.vdc};
		std::vector<std::pair<uint8_t, Symbol>> *ac[] = {&b.yac, &b.uac, &b.vac};
		Huffman *dcCoder[] = {&yDcHuff, &uvDcHuff, &uvDcHuff};
		Huffman *acCoder[] = {&yAcHuff, &uvAcHuff, &uvAcHuff};
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
				s.Get(tmp);
				readData.length++;
				readData.val = readData.val << 1 | tmp.val;
				if (readData.length > 16)
				{
					fprintf(stderr, "[Error] Huffman decode error, dc decode error\n");
					return -1;
				}
			} while (dcCoder[color]->Decode(readData, dcLength));
			readData.length = dcLength;
			s.Get(readData);
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
					s.Get(tmp);
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
					ac[color]->emplace_back(acVal, Symbol{0, 0});
					continue;
				}
				if (acVal == 0)
				{
					//EOB end of block
					ac[color]->emplace_back(0x00, Symbol{0, 0});
					break;
				}
				int zeroNum = acVal >> 4;
				readData.length = acVal & 0x0f;
				s.Get(readData);
				numValue += zeroNum + 1;
				ac[color]->emplace_back(acVal, readData);
			}
		}
		blockId++;
	}
	return 0;
}

Symbol GetVLI(int val)
{
	if (val == 0)
		return {0, 0};
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

int DeVLI(Symbol s)
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

void Huffman::Add(int val)
{
	if (val >= 0 && val < 256)
		frequence[val]++;
	else
	{
		fprintf(stderr, "[Warning] Huffman add warning, val >= 256 or <0\n");
	}
}
void Huffman::SetTable(uint8_t *bitSize, uint8_t *bitTable, const int tableLength)
{
	for (int i = 0; i < 16; ++i)
	{
		(this->bitSize)[i] = bitSize[i];
	}
	this->bitTable.assign(tableLength, 0);
	for (int i = 0; i < tableLength; ++i)
	{
		(this->bitTable)[i] = bitTable[i];
	}
}
/*
	Build huffman table (the code order and each code size)
	Follow ISO/IEC 10918-1:1993(E)(ITU-T81) K.2
	*/
void Huffman::BuildTable()
{
	struct SymbolAndFre
	{
		int frequence;
		int symbol;
		bool operator<(const SymbolAndFre &s) const { return frequence < s.frequence; }
		bool operator>(const SymbolAndFre &s) const { return frequence > s.frequence; }
	};
	std::vector<SymbolAndFre> symbols(257);
	symbols.clear();
	for (int i = 0; i < 257; ++i)
	{
		if (frequence[i] > 0)
			symbols.emplace_back(SymbolAndFre{frequence[i], i});
	}
	/*Because all one code is not allowed, 
		symbol 256 is added to avolid it.
		If there must be all one code, 256 will get it
		so it would not apear in file;
		*/
	symbols.emplace_back(SymbolAndFre{0, 256});
	std::sort(symbols.begin(), symbols.end(), std::greater<SymbolAndFre>());
	for (auto &s : symbols)
		if (s.frequence != 0)
			bitTable.emplace_back(s.symbol);
	std::priority_queue<SymbolAndFre, std::vector<SymbolAndFre>, std::greater<SymbolAndFre>> q(symbols.begin(), symbols.end());
	std::vector<int> codeSize, others;
	codeSize.assign(257, 0);
	others.assign(257, -1);
	while (q.size() > 1)
	{
		SymbolAndFre m1 = q.top();
		q.pop();
		SymbolAndFre m2 = q.top();
		q.pop();
		m1.frequence += m2.frequence;
		int v1, v2;
		for (v1 = m1.symbol; others[v1] != -1; v1 = others[v1])
		{
			codeSize[v1]++;
		}
		codeSize[v1]++;
		others[v1] = m2.symbol;
		for (v2 = m2.symbol; v2 != -1; v2 = others[v2])
		{
			codeSize[v2]++;
		}
		q.push(m1);
	}
	int bitLength = *std::max_element(codeSize.begin(), codeSize.end());
	std::vector<int> bits(bitLength + 1, 0);
	for (auto &val : codeSize)
	{
		if (val > 0)
			bits[val]++;
	}
	while (bitLength > 16)
	{
		int &i = bitLength;
		int j = i - 2;
		if (bits[i] == 0)
			i--;
		while (j > 0 && bits[j] == 0)
			j--;
		bits[i] -= 2;
		bits[i - 1]++;
		bits[j + 1] += 2;
		bits[j]--;
	}
	for (int tail = bitLength; tail > 0; --tail)
	{
		if (bits[tail] != 0)
		{
			bits[tail]--;
			break;
		}
	}
	for (unsigned i = 0; i < 16; ++i)
	{
		if (i + 1 >= bits.size())
		{
			bitSize[i] = 0;
		}
		else
		{
			bitSize[i] = bits[i + 1];
		}
	}
}
void Huffman::PrintTable()
{
	if (!CheckSize())
		return;
	int idx = 0;
	for (int i = 0; i < 16; ++i)
	{
		printf("%d : \t", i + 1);
		for (int j = 0; j < bitSize[i]; ++j)
		{
			printf("%X ", bitTable[idx++]);
		}
		printf("\n");
	}
	printf("Total size = %d\n", bitTable.size());
}
int Huffman::Encode(uint8_t source, Symbol &target)
{
	if (symbolMap.empty())
	{
		BuildSymbolMap();
	}
	std::map<int, Symbol>::iterator it = symbolMap.find(source);
	if (it == symbolMap.end())
	{
		fprintf(stderr, "[Error] Huffman encode error, unkonw symbol\n");
		return -1;
	}
	target = it->second;
	return 0;
}
int Huffman::Decode(Symbol &source, uint8_t &target)
{
	if (valueMap.empty())
		BuildValueMap();
	std::map<Symbol, int>::iterator it = valueMap.find(source);
	if (it == valueMap.end())
		return -1;
	target = it->second;
	return 0;
}
void Huffman::BuildSymbolMap()
{
	symbolMap.clear();
	if (!CheckSize())
	{
		return;
	}
	Symbol s;
	s.length = 1;
	s.val = 0;
	int idx = 0;
	for (int i = 0; i < 16; ++i)
	{
		for (int j = 0; j < bitSize[i]; ++j)
		{
			symbolMap.emplace(bitTable[idx++], s);
			s.val++;
		}
		s.length++;
		s.val <<= 1;
	}
}
void Huffman::BuildValueMap()
{
	symbolMap.clear();
	if (!CheckSize())
	{
		return;
	}
	Symbol s;
	s.length = 1;
	s.val = 0;
	int idx = 0;
	for (int i = 0; i < 16; ++i)
	{
		for (int j = 0; j < bitSize[i]; ++j)
		{
			valueMap.emplace(s, bitTable[idx++]);
			s.val++;
		}
		s.length++;
		s.val <<= 1;
	}
}
bool Huffman::CheckSize()
{
	int val = 2;
	for (int i = 0; i < 16; ++i)
	{
		if (val < 0)
		{
			fprintf(stderr, "[Error] Huffman tree error\n");
			return false;
		}
		val -= bitSize[i];
		val <<= 1;
	}
	return true;
}
}; //namespace jpeg