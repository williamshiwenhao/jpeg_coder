#pragma once
#ifndef __JPEG_H__
#define __JPEG_H__
#include <cmath>
#include <unordered_map>
#include <queue>

#include "bmp.h"
#include "types.h"
#include "utilities.h"
#include "huffman.h"

namespace jpeg {

	

	int * GetYTable(int level);
	int * GetUvTable(int level);

	

	class JpegCoder {
	public:
		int Encode(const Img<Rgb>& s, int level);
		int Decode(Img<Rgb>& t);
		int Write(const char* fileName);
		int Read(const char* fileName);

	private:
		/*Function*/
		/*Write to file **********************************************************/
		inline void Write16(uint16_t val) {
			uint8_t res;
			res = (val >> 8)& 0xff;
			data.push_back(res);
			res = val & 0xff;
			data.push_back(res);
		}
		inline uint16_t Read16(int idx) {
			uint16_t ans = 0;
			uint16_t res1, res2;
			res1 = data[idx];
			res2 = data[idx + 1];
			ans = (res1 << 8) | (res2 & 0xff);
			return ans;
		}
		void EncodePicHead();
		void EncodeQuantTable(int level = 0);
		void EncodePicInfo(uint16_t w, uint16_t);
		void EncodeHuffmanTable(const int* bitSize, const std::vector<int>& bitTable, bool isDc, uint8_t htid);
		void EncodeImg(uint8_t *s, int length);

		int DecodeQuantTable(int base, int length);
		int DecodePicInfo(int base, int length, int&w, int&h);
		int DecodeHuffmanTable(int base, int length);
		int DecodeImg(int base, int length);
		
		/*Encode and decode algorithm*/
		ImgBlock<double> FDCT(ImgBlock<double>);
		ImgBlock<double> FIDCT(ImgBlock<double>);
		template<class T> ImgBlock<int> Quant(T& block, int level = 0);
		template<class T> ImgBlock<double> Iquant(T& block);
		template<class T> void ZigZag(ImgBlock<T>& block);
		template<class T> void IZigZag(ImgBlock<T>& block);
		ImgBlockCode RunLengthCode(const ImgBlock<int> &block);
		ImgBlock<int> RunLengthDecode(const ImgBlockCode &code);
		void BuildHuffman(ImgBlockCode &block);
		int HuffmanEncode(ImgBlockCode &block, BitStream &stream);
		int HuffmanDecode(BitStream &s, ImgBlockCode &code);
		Symbol GetVLI(int val);
		int DeVLI(Symbol s);
		/*Data*/
		std::vector<uint8_t> data;//Encoded data, write to file or read from file
		std::vector<uint8_t> imgData;
		Huffman yDcHuff, yAcHuff, uvDcHuff, uvAcHuff;//4 Huffman table
		Huffman dcHuff[4], acHuff[4];
		std::vector<int> qTable[16];//Quant table for decode, read from file. Not for encode!
		std::vector<int>* pYTable, *pUvTable;
	};

};//namespace jpeg

#endif/*__JPEG_H__*/