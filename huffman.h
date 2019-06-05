#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__
#include <map>
#include <unordered_map>
#include <queue>

#include "jpeg.h"
#include "types.h"

namespace jpeg
{
class Huffman
{
public:
	/*Add a value to Huffman tree, statistic frequences*/
	void Add(int val);
	void SetTable(uint8_t *bitSize, uint8_t *bitTable, const int tableLength);
	void BuildTable();
	void PrintTable();
	int Encode(uint8_t source, Symbol &target);
	int Decode(Symbol &source, uint8_t &target);
	const int* GetSize()const;
	const std::vector<int>& GetTable()const;

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
ImgBlockCode RunLengthCode(const ImgBlock<int> &block);
ImgBlock<int> RunLengthDecode(const ImgBlockCode &code);
void BuildHuffman(ImgBlockCode &block, Huffman &yDcHuff, Huffman &yAcHuff, Huffman &uvDcHuff, Huffman &uvAcHuff);
int HuffmanEncode(ImgBlockCode &block, BitStream &stream, Huffman &yDcHuff, Huffman &yAcHuff, Huffman &uvDcHuff, Huffman &uvAcHuff);
int HuffmanDecode(BitStream &s, ImgBlockCode &code, Huffman &yDcHuff, Huffman &yAcHuff, Huffman &uvDcHuff, Huffman &uvAcHuff);
Symbol GetVLI(int val);
int DeVLI(Symbol s);

}; //namespace jpeg

#endif //__HUFFMAN_H__