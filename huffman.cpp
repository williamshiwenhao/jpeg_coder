#include "huffman.h"
#include "utilities.h"

#include <queue>
#include <string>
#include <algorithm>

namespace jpeg
{
	void BitStream::PushBack(const uint8_t &val)
	{
		data.push_back(val);
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

	//************************************
	// Method:    Get
	// FullName:  jpeg::BitStream::Get
	// Access:    public
	// Returns:   int,0 for normal return, and -2 for meet end£¬ -1
	// Qualifier:
	// Parameter: Symbol & s
	//************************************
	int BitStream::Get(Symbol &s)
	{
		if (s.length > 16 || s.length < 0)
		{
			fprintf(stderr, "[Error] BitStream get length %d\n", s.length);
			return -1;
		}
		s.val = 0;
		int remainBit = 8 - head;
		if (remainBit >= s.length)
		{
			if (readIdx >= data.size())
			{
				//more than have, next stream
				return -2;
			}
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
			if (readIdx >= data.size()) {
				//want more than have
				return -2;
			}
			head = 0;
			while (s.length - sPos >= 8)
			{
				if (readIdx >= data.size())
				{
					return -2;
				}
				SetBitSymbol(s, sPos, 8, data[readIdx++]);
				sPos += 8;
			}
			if (s.length - sPos > 0)
			{
				if (readIdx >= data.size())
				{
					return -2;
				}
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

	void BitStream::SetData(const std::vector<uint8_t> &s)
	{
		data = s;
		tail = 8;
		head = 0;
		readIdx = 0;
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
	void Huffman::SetTable(const uint8_t *bitSize, const uint8_t *bitTable, const int tableLength)
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
				symbols.emplace_back(SymbolAndFre{ frequence[i], i });
		}
		/*Because all one code is not allowed,
				symbol 256 is added to avolid it.
				If there must be all one code, 256 will get it
				so it would not apear in file;
				*/
		symbols.emplace_back(SymbolAndFre{ 0, 256 });
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
	const int *Huffman::GetSize() const
	{
		return bitSize;
	}
	const std::vector<int> &Huffman::GetTable() const
	{
		return bitTable;
	}
	Huffman &Huffman::operator=(const Huffman &s)
	{
		memcpy(bitSize, s.bitSize, sizeof(bitSize));
		bitTable = s.bitTable;
		symbolMap.clear();
		valueMap.clear();
		return *this;
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