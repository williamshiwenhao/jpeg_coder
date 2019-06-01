#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__
#include <map>
#include <unordered_map>
#include <queue>

#include "jpeg.h"
#include "types.h"

namespace jpeg {
	ImgBlockCode RunLengthCode(const ImgBlock<int>& block);
	ImgBlock<int> RunLengthDecode(const ImgBlockCode& code);
	int HuffmanEncode(ImgBlockCode &, BitStream&);
	Symbol GetVLI(int val);
	int DeVLI(Symbol s);
	class Huffman {
	public:
		/*Add a value to Huffman tree, statistic frequences*/
		void Add(int val);
		void SetTable(uint8_t* bitSize, uint8_t* bitTable, const int tableLength);
		void BuildTable();
		void PrintTable();
		int Encode(uint8_t source, Symbol& target);
		int Decode(Symbol &source, uint8_t &target);
	private:
		void BuildSymbolMap();
		void BuildValueMap();
		bool CheckSize();
		int frequence[257] = {};
		int bitSize[16];//number of code of length idx 
		std::vector<int> bitTable;
		std::map<int, Symbol> symbolMap;//for encode, value to symbol
		std::map<Symbol, int> valueMap;//for decode, symbol to value
	};
};//namespace jpeg

#endif//__HUFFMAN_H__