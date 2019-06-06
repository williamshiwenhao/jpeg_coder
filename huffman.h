#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__
#include <map>
#include <unordered_map>
#include <queue>

#include "types.h"

namespace jpeg
{
	class BitStream {
	public:
		void Add(const Symbol& s);
		int Get(Symbol& s);
		int print();//for debug
		int GetData(uint8_t**);
		void SetData(std::vector<uint8_t>::iterator begin, std::vector<uint8_t>::iterator end);
	private:
		std::vector<uint8_t> data;
		int tail = 8;
		int head = 0;
		unsigned int readIdx = 0;

	};

class Huffman
{
public:
	/*Add a value to Huffman tree, statistic frequences*/
	void Add(int val);
	void SetTable(const uint8_t *bitSize, const uint8_t *bitTable, const int tableLength);
	void BuildTable();
	void PrintTable();
	int Encode(uint8_t source, Symbol &target);
	int Decode(Symbol &source, uint8_t &target);
	const int* GetSize()const;
	const std::vector<int>& GetTable()const;
	Huffman& operator=(const Huffman& s);

private:
	void BuildSymbolMap();
	void BuildValueMap();
	bool CheckSize();
	int frequence[257] = {};
	int bitSize[16]; //number of code of length idx
	std::vector<int> bitTable;
	std::map<int, Symbol> symbolMap; //for encode, value to symbol
	std::map<Symbol, int> valueMap;  //for decode, symbol to value
};



}; //namespace jpeg

#endif //__HUFFMAN_H__