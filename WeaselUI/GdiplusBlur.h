#pragma once
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

namespace weasel{
	void boxesForGauss(double sigma, int* sizes, int n);
	void boxBlurH_4(BYTE* scl, BYTE* tcl, int w, int h, int r, int bpp, int stride);
	void boxBlurT_4(BYTE* scl, BYTE* tcl, int w, int h, int r, int bpp, int stride);
	void boxBlur_4(BYTE* scl, BYTE* tcl, int w, int h, int rx, int ry, int bpp, int stride);
	void gaussBlur_4(BYTE* scl, BYTE* tcl, int w, int h, float rx, float ry, int bpp, int stride);
	void DoGaussianBlur(Gdiplus::Bitmap* img, float radiusX, float radiusY);
	void DoGaussianBlurPower(Gdiplus::Bitmap* img, float radiusX, float radiusY, int nPower);
}
