#ifndef __HUFFMAN_H__
#define __HUFFMAN_H__
#include <map>
#include <unordered_map>
#include <queue>

#include "jpeg.h"
#include "types.h"

namespace jpeg {
	ImgBlockCode RunLengthCoding(ImgBlock<int>& block);
	Symbol GetVLI(int val);
	int DeVLI(Symbol s);
	class Huffman {
	public:
		void Add(int val);
	private:
		std::unordered_map<int, int> frequence;
	};
};//namespace jpeg

#endif//__HUFFMAN_H__