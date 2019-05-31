#include "huffman.h"

namespace jpeg {
	ImgBlockCode RunLengthCode(const ImgBlock<int>& block) {
		ImgBlockCode code;
		code.w = block.w;
		code.h = block.h;
		code.wb = block.wb;
		code.hb = block.hb;
		code.data = std::vector<BlockCode>(code.wb * code.hb);
		int lastYDc = 0, lastUDc = 0, lastVDc = 0;
		for (int i = 0; i < block.wb * block.hb; ++i) {
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
			const int* s[] = { block.data[i].y, block.data[i].u, block.data[i].v };
			std::vector<std::pair<uint8_t, Symbol> >* t[] = { &code.data[i].yac, &code.data[i].uac, &code.data[i].vac };
			for (int color = 0; color < 3; ++color) {
				int zeroNum = 0;
				auto ss = s[color];
				auto tt = t[color];
				//find eob
				int eob = 0;
				for (int i = 63; i >= 0; --i) {
					if (ss[i] != 0) {
						eob = i;
						break;
					}
				}
				for (int i = 1; i <=eob; ++i) {
					if (ss[i] == 0) {
						zeroNum++;
						if (zeroNum == 16) {
							zeroNum = 0;
							tt->emplace_back(0xf0, Symbol{0,0});
						}
					}
					else {
						Symbol val = GetVLI(ss[i]);
						uint8_t head = 0;
						head |= val.length & 0x0f;
						head |= (zeroNum & 0x0f) << 4;
						tt->emplace_back(head, val);
						zeroNum = 0;
					}
				}
				if (eob < 63) {
					tt->emplace_back(0x00, Symbol{0,0});
				}
			}
		}
		return code;
	}

	ImgBlock<int> RunLengthDecode(const ImgBlockCode& code) {
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
		for (int i = 0; i < size; ++i) {
			lastYDc += DeVLI(code.data[i].ydc);
			lastUDc += DeVLI(code.data[i].udc);
			lastVDc += DeVLI(code.data[i].vdc);
			block.data[i].y[0] = lastYDc;
			block.data[i].u[0] = lastUDc;
			block.data[i].v[0] = lastVDc;
			int* t[] = { block.data[i].y, block.data[i].u, block.data[i].v };
			const std::vector<std::pair<uint8_t, Symbol> >* s[] = { &code.data[i].yac, &code.data[i].uac, &code.data[i].vac };
			for (int color = 0; color < 3; ++color) {
				int* tt = t[color];
				const std::vector<std::pair<uint8_t, Symbol>>& ss = *s[color];
				int idx = 1;
				for (auto &p : ss) {
					uint8_t head = p.first;
					Symbol data = p.second;
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

	Symbol GetVLI(int val) {
		if (val == 0)
			return { 0, 0 };
		int middle = (val > 0) ? val: -val;
		Symbol ans; 
		ans.length = 0;
		while (middle >> ans.length > 0) {
			ans.length++;
		}
		if (ans.length > 11) {
			fprintf(stderr, "[Warning] VLI value >= 2048\n");
		}
		if (val < 0)
			ans.val = (~middle) & ((1 << ans.length) - 1);
		else
			ans.val = middle;
		return ans;
	}

	int DeVLI(Symbol s) {
		if (s.length == 0)
			return 0;
		if (s.val >> (s.length - 1) & 0x01) {
			return s.val;
		}
		int ans = ((~s.val) & ((1 << s.length) - 1));
		return -ans;
	}
};//namespace jpeg