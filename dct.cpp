#include <cmath>
#include <vector>
#include "utilities.h"
#include "jpeg.h"

namespace jpeg {
	void DCTCore(double *);
	void IDCTCore(double *);
	void FDCTRaw(int w, int h, std::vector<double> &data, void(*core)(double*));

	ImgChannel FDCT(Img<Yuv> img) {
		ImgChannel ch = Img2Channel(img);
		FDCTRaw(ch.w, ch.h, ch.y, DCTCore);
		FDCTRaw(ch.w, ch.h, ch.cb, DCTCore);
		FDCTRaw(ch.w, ch.h, ch.cr, DCTCore);
		return ch;
	}

	Img<Yuv> FIDCT(ImgChannel& ch) {
		FDCTRaw(ch.w, ch.h, ch.y, IDCTCore);
		FDCTRaw(ch.w, ch.h, ch.cb, IDCTCore);
		FDCTRaw(ch.w, ch.h, ch.cr, IDCTCore);
		return Channel2Img(ch);
	}

	void FDCTRaw(int w, int h, std::vector<double> &data, void(*core)(double*)) {
		if (data.size() != w * h) {
			fprintf(stderr, "[Error] DCT error, w*h != data.size\n");
			return;
		}
		double res[8];
		int wb = ceil(double(w) / 8.0);
		int hb = ceil(double(h) / 8.0);
		auto dctBlock = [&](int xb, int yb) {
			//row
			for (int y = yb * 8; y < yb * 8 + 8; ++y) {
				for (int i = 0; i < 8; ++i) {
					int x = xb * 8 + i;
					if (x >= w || y >= h) {
						res[i] = 0;
					}
					else {
						int idx = y * w + x;
						res[i] = data[idx];
					}
				}
				core(res);
				for (int i = 0; i < 8; ++i) {
					int x = xb * 8 + i;
					if (x < w && y < h) {
						int idx = y * w + x;
						data[idx] = res[i];
					}
				}
			}
			//colom
			for (int x = xb * 8; x < xb * 8 + 8; ++x) {
				for (int i = 0; i < 8; ++i) {
					int y = yb * 8 + i;
					if (x >= w || y >= h) {
						res[i] = 0;
					}
					else {
						int idx = y * w + x;
						res[i] = data[idx];
					}
				}
				core(res);
				for (int i = 0; i < 8; ++i) {
					int y = yb * 8 + i;
					if (x < w && y < h) {
						int idx = y * w + x;
						data[idx] = res[i];
					}
				}
			}
		};
		for (int xb = 0; xb < wb; xb++) {
			for (int yb = 0; yb < hb; yb++) {
				dctBlock(xb, yb);
			}
		}
	}

	static const double S[] = {
	0.353553390593273762200422,
	0.254897789552079584470970,
	0.270598050073098492199862,
	0.300672443467522640271861,
	0.353553390593273762200422,
	0.449988111568207852319255,
	0.653281482438188263928322,
	1.281457723870753089398043,
	};

	static const double A[] = {
		NAN,
		0.707106781186547524400844,
		0.541196100146196984399723,
		0.707106781186547524400844,
		1.306562964876376527856643,
		0.382683432365089771728460,
	};
	// DCT AAN
	void DCTCore(double *vector) {
		const double v0 = vector[0] + vector[7];
		const double v1 = vector[1] + vector[6];
		const double v2 = vector[2] + vector[5];
		const double v3 = vector[3] + vector[4];
		const double v4 = vector[3] - vector[4];
		const double v5 = vector[2] - vector[5];
		const double v6 = vector[1] - vector[6];
		const double v7 = vector[0] - vector[7];

		const double v8 = v0 + v3;
		const double v9 = v1 + v2;
		const double v10 = v1 - v2;
		const double v11 = v0 - v3;
		const double v12 = -v4 - v5;
		const double v13 = (v5 + v6) * A[3];
		const double v14 = v6 + v7;

		const double v15 = v8 + v9;
		const double v16 = v8 - v9;
		const double v17 = (v10 + v11) * A[1];
		const double v18 = (v12 + v14) * A[5];

		const double v19 = -v12 * A[2] - v18;
		const double v20 = v14 * A[4] - v18;

		const double v21 = v17 + v11;
		const double v22 = v11 - v17;
		const double v23 = v13 + v7;
		const double v24 = v7 - v13;

		const double v25 = v19 + v24;
		const double v26 = v23 + v20;
		const double v27 = v23 - v20;
		const double v28 = v24 - v19;

		vector[0] = S[0] * v15;
		vector[1] = S[1] * v26;
		vector[2] = S[2] * v21;
		vector[3] = S[3] * v28;
		vector[4] = S[4] * v16;
		vector[5] = S[5] * v25;
		vector[6] = S[6] * v22;
		vector[7] = S[7] * v27;
	}


	// A straightforward inverse of the forward algorithm.
	void IDCTCore(double *vector) {
		const double v15 = vector[0] / S[0];
		const double v26 = vector[1] / S[1];
		const double v21 = vector[2] / S[2];
		const double v28 = vector[3] / S[3];
		const double v16 = vector[4] / S[4];
		const double v25 = vector[5] / S[5];
		const double v22 = vector[6] / S[6];
		const double v27 = vector[7] / S[7];

		const double v19 = (v25 - v28) / 2;
		const double v20 = (v26 - v27) / 2;
		const double v23 = (v26 + v27) / 2;
		const double v24 = (v25 + v28) / 2;

		const double v7 = (v23 + v24) / 2;
		const double v11 = (v21 + v22) / 2;
		const double v13 = (v23 - v24) / 2;
		const double v17 = (v21 - v22) / 2;

		const double v8 = (v15 + v16) / 2;
		const double v9 = (v15 - v16) / 2;

		const double v18 = (v19 - v20) * A[5];  // Different from original
		const double v12 = (v19 * A[4] - v18) / (A[2] * A[5] - A[2] * A[4] - A[4] * A[5]);
		const double v14 = (v18 - v20 * A[2]) / (A[2] * A[5] - A[2] * A[4] - A[4] * A[5]);

		const double v6 = v14 - v7;
		const double v5 = v13 / A[3] - v6;
		const double v4 = -v5 - v12;
		const double v10 = v17 / A[1] - v11;

		const double v0 = (v8 + v11) / 2;
		const double v1 = (v9 + v10) / 2;
		const double v2 = (v9 - v10) / 2;
		const double v3 = (v8 - v11) / 2;

		vector[0] = (v0 + v7) / 2;
		vector[1] = (v1 + v6) / 2;
		vector[2] = (v2 + v5) / 2;
		vector[3] = (v3 + v4) / 2;
		vector[4] = (v3 - v4) / 2;
		vector[5] = (v2 - v5) / 2;
		vector[6] = (v1 - v6) / 2;
		vector[7] = (v0 - v7) / 2;
	}

};//namespace jpeg