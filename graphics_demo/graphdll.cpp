
/*
Copyright (c) 2013, Johan Sarge
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
	
	1. Redistributions of source code must retain the above copyright notice,
	this list of conditions and the following disclaimer.
	
	2. Redistributions in binary form must reproduce the above copyright notice,
	this list of conditions and the following disclaimer in the documentation
	and/or other materials provided with the distribution.
	
	3. Neither the name of the copyright holder nor the names of its contributors
	may be used to endorse or promote products derived from this software without
	specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include "graphdll.hpp"

namespace { // begin anonymous namespace

// Data Definition
const int PIXEL_BUFFER_SIZE = 4 * 1024;
const double RECIPROCAL_255 = 1.0 / 255.0;

const double LUMA_COEFF_R_REC709 = 0.2126;
const double LUMA_COEFF_G_REC709 = 0.7152;
const double LUMA_COEFF_B_REC709 = 0.0722;

const double LUMA_COEFF_R = LUMA_COEFF_R_REC709;
const double LUMA_COEFF_G = LUMA_COEFF_G_REC709;
const double LUMA_COEFF_B = LUMA_COEFF_B_REC709;

const double TINY_LUMA_COEFF_R = RECIPROCAL_255 * LUMA_COEFF_R;
const double TINY_LUMA_COEFF_G = RECIPROCAL_255 * LUMA_COEFF_G;
const double TINY_LUMA_COEFF_B = RECIPROCAL_255 * LUMA_COEFF_B;

const std::string EMPTY_STRING;


// Helper Functions
template<typename T>
void minMax3(T v1, T v2, T v3, T &vmin, T &vmax) {
	T vmid;
	
	if (v2 <= v3) {
		vmin = v2;
		vmid = v3;
	}
	else {
		vmin = v3;
		vmid = v2;
	}
	
	if (v1 <= vmid) {
		vmax = vmid;
		if (v1 <= vmin)
			vmin = v1;
	}
	else
		vmax = v1;
}

unsigned int doubleTo8bit(double d) {
	unsigned int i;
	if (d > 1.0)
		i = 255;
	else if (d < 0.0)
		i = 0;
	else
		i = (unsigned int)(255.0*d + 0.5);
	return i;
}

double doubleFrom8bit(unsigned int i) {
	//return (i >= 255) ? 1.0 : RECIPROCAL_255 * (double)i;
	return RECIPROCAL_255 * (double)i;
}

void convertRGBtoHCL(double r, double g, double b, double &h, double &c, double &l) {
	double m0, m1;
	minMax3(r, g, b, m0, m1);
	
	c = m1 - m0;
	
	if (c <= 0.0)
		h = -100.0; // Hue is undefined.
	else {
		if (m1 == r)
			h = std::fmod((g - b) + 6.0*c,  6.0*c);
		else if (m1 == g)
			h = (b - r) + 2.0*c;
		else
			h = (r - g) + 4.0*c;
		
		h /= c;
	}
	
	l = LUMA_COEFF_R*r + LUMA_COEFF_G*g + LUMA_COEFF_B*b;
}

void convertHCLtoRGB(double h, double c, double l, double &r, double &g, double &b) {
	double x = c * (1.0 - std::fabs(std::fmod(h, 2.0) - 1.0));
	
	if (h < 0.0)      { r = 0.0; g = 0.0; b = 0.0; } // Undefined hue, i.e. gray.
	else if (h < 1.0) { r = c;   g = x;   b = 0.0; }
	else if (h < 2.0) { r = x;   g = c;   b = 0.0; }
	else if (h < 3.0) { r = 0.0; g = c;   b = x; }
	else if (h < 4.0) { r = 0.0; g = x;   b = c; }
	else if (h < 5.0) { r = x;   g = 0.0; b = c; }
	else if (h < 6.0) { r = c;   g = 0.0; b = x; }
	else              { r = 0.0; g = 0.0; b = 0.0; } // Undefined hue, i.e. gray.
	
	double m = l - (LUMA_COEFF_R*r + LUMA_COEFF_G*g + LUMA_COEFF_B*b);
	r += m; g += m; b += m;
}

void getExtension(const std::string &path, std::string &extension) {
	extension.clear();
	
	std::string::size_type dotIndex = path.find_last_of('.');
	if (dotIndex == std::string::npos || dotIndex == path.size()-1)
		return;
	
	extension.assign(path, dotIndex+1, path.size() - dotIndex);
}

int getImageFormat(const std::string &path, const std::string &type) {
	std::string fileType = type;
	if (fileType == EMPTY_STRING)
		getExtension(path, fileType);
	
	int imageFormat;
	if (fileType == "bmp" || fileType == "Bmp" || fileType == "BMP")
		imageFormat = CG_FILE_FORMAT_BMP;
	else
		imageFormat = CG_FILE_FORMAT_NONE;
	
	return imageFormat;
}


// Channel Extraction
typedef void (*Extractor)(unsigned int pixel, void *&channel);

void redExtractor(unsigned int pixel, void *&channel) {
	double *dchan = (double*)channel;
	*dchan = doubleFrom8bit(0xffU & pixel >> 16);
	channel = dchan+1;
}

void greenExtractor(unsigned int pixel, void *&channel) {
	double *dchan = (double*)channel;
	*dchan = doubleFrom8bit(0xffU & pixel >> 8);
	channel = dchan+1;
}

void blueExtractor(unsigned int pixel, void *&channel) {
	double *dchan = (double*)channel;
	*dchan = doubleFrom8bit(0xffU & pixel);
	channel = dchan+1;
}

void alphaExtractor(unsigned int pixel, void *&channel) {
	double *dchan = (double*)channel;
	*dchan = doubleFrom8bit(0xffU & pixel >> 24);
	channel = dchan+1;
}

void redByteExtractor(unsigned int pixel, void *&channel) {
	uchar *bchan = (uchar*)channel;
	*bchan = (uchar)(0xffU & pixel >> 16);
	channel = bchan+1;
}

void greenByteExtractor(unsigned int pixel, void *&channel) {
	uchar *bchan = (uchar*)channel;
	*bchan = (uchar)(0xffU & pixel >> 8);
	channel = bchan+1;
}

void blueByteExtractor(unsigned int pixel, void *&channel) {
	uchar *bchan = (uchar*)channel;
	*bchan = (uchar)(0xffU & pixel);
	channel = bchan+1;
}

void alphaByteExtractor(unsigned int pixel, void *&channel) {
	uchar *bchan = (uchar*)channel;
	*bchan = (uchar)(0xffU & pixel >> 24);
	channel = bchan+1;
}

void hueExtractor(unsigned int pixel, void *&channel) {
	double *dchan = (double*)channel;
	int r = 0xffU & pixel >> 16;
	int g = 0xffU & pixel >> 8;
	int b = 0xffU & pixel;
	int m0, m1;
	minMax3(r, g, b, m0, m1);
	int c = m1 - m0;
	int h;
	
	if (c == 0)
		*dchan = -100.0; // Hue is undefined.
	else {
		if (m1 == r)
			h = ((g - b) + 6*c) % (6*c);
		else if (m1 == g)
			h = (b - r) + 2*c;
		else
			h = (r - g) + 4*c;
		
		*dchan = (double)h / (double)c;
	}
	
	channel = dchan+1;
}

void chromaExtractor(unsigned int pixel, void *&channel) {
	double *dchan = (double*)channel;
	int r = 0xffU & pixel >> 16;
	int g = 0xffU & pixel >> 8;
	int b = 0xffU & pixel;
	int m0, m1;
	minMax3(r, g, b, m0, m1);
	*dchan = RECIPROCAL_255 * (double)(m1 - m0);
	channel = dchan+1;
}

void lumaExtractor(unsigned int pixel, void *&channel) {
	double *dchan = (double*)channel;
	int r = 0xffU & pixel >> 16;
	int g = 0xffU & pixel >> 8;
	int b = 0xffU & pixel;
	*dchan =
		TINY_LUMA_COEFF_R * (double)r +
		TINY_LUMA_COEFF_G * (double)g +
		TINY_LUMA_COEFF_B * (double)b;
	channel = dchan+1;
}

void hueByteExtractor(unsigned int pixel, void *&channel) {
	uchar *bchan = (uchar*)channel;
	double d;
	void *dp = &d;
	hueExtractor(pixel, dp);
	*bchan = (uchar)doubleTo8bit(d / 6.0);
	channel = bchan+1;
}

void chromaByteExtractor(unsigned int pixel, void *&channel) {
	uchar *bchan = (uchar*)channel;
	double d;
	void *dp = &d;
	chromaExtractor(pixel, dp);
	*bchan = (uchar)doubleTo8bit(d);
	channel = bchan+1;
}

void lumaByteExtractor(unsigned int pixel, void *&channel) {
	uchar *bchan = (uchar*)channel;
	double d;
	void *dp = &d;
	lumaExtractor(pixel, dp);
	*bchan = (uchar)doubleTo8bit(d);
	channel = bchan+1;
}

void set1Extractor(unsigned int pixel, void *&channel) {
	double *dchan = (double*)channel;
	*dchan = 1.0;
	channel = dchan+1;
}

void nopExtractor(unsigned int pixel, void *&channel) {}


// Pixel Packing
typedef unsigned int (*Packer3)(const void *&r, const void *&g, const void *&b);

unsigned int packRGB(const void *&r, const void *&g, const void *&b) {
	const double *dr = (const double*)r, *dg = (const double*)g, *db = (const double*)b;
	unsigned int ri = doubleTo8bit(*dr);
	unsigned int gi = doubleTo8bit(*dg);
	unsigned int bi = doubleTo8bit(*db);
	r = dr+1; g = dg+1; b = db+1;
	return ri << 16 | gi << 8 | bi;
}

unsigned int packBytesRGB(const void *&r, const void *&g, const void *&b) {
	const uchar *dr = (const uchar*)r, *dg = (const uchar*)g, *db = (const uchar*)b;
	unsigned int ri = *dr;
	unsigned int gi = *dg;
	unsigned int bi = *db;
	r = dr+1; g = dg+1; b = db+1;
	return ri << 16 | gi << 8 | bi;
}

unsigned int packHCLinRGB(const void *&h, const void *&c, const void *&l) {
	const double *dh = (const double*)h, *dc = (const double*)c, *dl = (const double*)l;
	double r, g, b;
	convertHCLtoRGB(*dh, *dc, *dl, r, g, b);
	unsigned int ri = doubleTo8bit(r);
	unsigned int gi = doubleTo8bit(g);
	unsigned int bi = doubleTo8bit(b);
	h = dh+1; c = dc+1; l = dl+1;
	return ri << 16 | gi << 8 | bi;
}

unsigned int packBytesHCLinRGB(const void *&h, const void *&c, const void *&l) {
	const uchar *dh = (const uchar*)h, *dc = (const uchar*)c, *dl = (const uchar*)l;
	double r, g, b;
	convertHCLtoRGB(6.0 * doubleFrom8bit(*dh), doubleFrom8bit(*dc), doubleFrom8bit(*dl), r, g, b);
	unsigned int ri = doubleTo8bit(r);
	unsigned int gi = doubleTo8bit(g);
	unsigned int bi = doubleTo8bit(b);
	h = dh+1; c = dc+1; l = dl+1;
	return ri << 16 | gi << 8 | bi;
}

typedef unsigned int (*Packer4)(const void *&r, const void *&g, const void *&b, const void *&a);

unsigned int packRGBA(const void *&r, const void *&g, const void *&b, const void *&a) {
	const double
		*dr = (const double*)r, *dg = (const double*)g,
		*db = (const double*)b, *da = (const double*)a;
	unsigned int ri = doubleTo8bit(*dr);
	unsigned int gi = doubleTo8bit(*dg);
	unsigned int bi = doubleTo8bit(*db);
	unsigned int ai = doubleTo8bit(*da);
	r = dr+1; g = dg+1; b = db+1; a = da+1;
	return ai << 24 | ri << 16 | gi << 8 | bi;
}

unsigned int packBytesRGBA(const void *&r, const void *&g, const void *&b, const void *&a) {
	const uchar
		*dr = (const uchar*)r, *dg = (const uchar*)g,
		*db = (const uchar*)b, *da = (const uchar*)a;
	unsigned int ri = *dr;
	unsigned int gi = *dg;
	unsigned int bi = *db;
	unsigned int ai = *da;
	r = dr+1; g = dg+1; b = db+1; a = da+1;
	return ai << 24 | ri << 16 | gi << 8 | bi;
}

unsigned int packHCLAinRGBA(const void *&h, const void *&c, const void *&l, const void *&a) {
	const double
		*dh = (const double*)h, *dc = (const double*)c,
		*dl = (const double*)l, *da = (const double*)a;
	double r, g, b;
	convertHCLtoRGB(*dh, *dc, *dl, r, g, b);
	unsigned int ri = doubleTo8bit(r);
	unsigned int gi = doubleTo8bit(g);
	unsigned int bi = doubleTo8bit(b);
	unsigned int ai = doubleTo8bit(*da);
	h = dh+1; c = dc+1; l = dl+1; a = da+1;
	return ai << 24 | ri << 16 | gi << 8 | bi;
}

unsigned int packBytesHCLAinRGBA(const void *&h, const void *&c, const void *&l, const void *&a) {
	const uchar
		*dh = (const uchar*)h, *dc = (const uchar*)c,
		*dl = (const uchar*)l, *da = (const uchar*)a;
	double r, g, b;
	convertHCLtoRGB(6.0 * doubleFrom8bit(*dh), doubleFrom8bit(*dc), doubleFrom8bit(*dl), r, g, b);
	unsigned int ri = doubleTo8bit(r);
	unsigned int gi = doubleTo8bit(g);
	unsigned int bi = doubleTo8bit(b);
	unsigned int ai = *da;
	h = dh+1; c = dc+1; l = dl+1; a = da+1;
	return ai << 24 | ri << 16 | gi << 8 | bi;
}


// Image File Read Access
int read24bitPixels(
	FILE *fptr, int width, int height,
	Extractor rx, Extractor gx, Extractor bx, Extractor ax,
	void *r, void *g, void *b, void *a,
	int &result)
{
	char *buffer = new char[PIXEL_BUFFER_SIZE];
	if (!buffer)
		return result = CGRESULT_ALLOC_FAILED;
	
	unsigned int pixelBytesPerRow = 3 * width;
	unsigned int padBytesPerRow = (pixelBytesPerRow % 4 == 0) ? 0 : 4 - pixelBytesPerRow % 4;
	unsigned int bytesPerRow = pixelBytesPerRow + padBytesPerRow;
	unsigned int totalBytes = bytesPerRow * height;
	unsigned int totalBytesRead = 0;
	void *rp = r, *gp = g, *bp = b, *ap = a;
	
	while (totalBytesRead < totalBytes) {
		unsigned int bytesRemaining = totalBytes - totalBytesRead;
		unsigned int rowIndex = totalBytesRead / bytesPerRow;
		unsigned int rowByteIndex = totalBytesRead % bytesPerRow;
		unsigned int columnIndex = rowByteIndex / 3;
		unsigned int bytesToRead =
			(PIXEL_BUFFER_SIZE < bytesRemaining) ? PIXEL_BUFFER_SIZE : bytesRemaining;
		int bytesExtracted = 0;
		
		// NOTE: This stuff is necessary because we must avoid ending the read in the middle
		// of a field (pixel or padding).
		unsigned int newTotalBytesRead = totalBytesRead + bytesToRead;
		unsigned int newRowByteIndex = newTotalBytesRead % bytesPerRow;
		if (newRowByteIndex < pixelBytesPerRow)              // Avoid split pixel.
			bytesToRead -= newRowByteIndex % 3;                // Read a multiple of 3 bytes of the row.
		else                                                 // Avoid split padding.
			bytesToRead -= newRowByteIndex - pixelBytesPerRow; // Do not read any of the pad bytes.
		
		int bytesRead = std::fread(buffer, 1, bytesToRead, fptr);
		if (bytesRead != bytesToRead) {
			result = CGRESULT_READ_ERROR;
			goto finish;
		}
		
		while (bytesExtracted < bytesRead) {
			while (columnIndex < width && bytesExtracted < bytesRead) {
				unsigned int pixel = 0xff000000U;
				std::memcpy(&pixel, buffer + bytesExtracted, 3);
				// NOTE: The extractors increment the output buffer pointers as necessary.
				rx(pixel, rp);
				gx(pixel, gp);
				bx(pixel, bp);
				ax(pixel, ap);
				columnIndex++;
				bytesExtracted += 3;
			}
			
			if (padBytesPerRow > 0 && bytesExtracted < bytesRead)
				bytesExtracted += padBytesPerRow;
			
			rowIndex++;
			columnIndex = 0;
		}
		
		totalBytesRead += bytesRead;
	}
	
	result = CGRESULT_OK;
	
finish:
	delete[] buffer;
	return result;
}

int read32bitPixels(
	FILE *fptr, int width, int height,
	Extractor rx, Extractor gx, Extractor bx, Extractor ax,
	void *r, void *g, void *b, void *a,
	int &result)
{
	char *buffer = new char[PIXEL_BUFFER_SIZE];
	if (!buffer)
		return result = CGRESULT_ALLOC_FAILED;
	
	unsigned int pixelBytesPerRow = 4 * width;
	unsigned int bytesPerRow = pixelBytesPerRow;
	unsigned int totalBytes = bytesPerRow * height;
	unsigned int totalBytesRead = 0;
	void *rp = r, *gp = g, *bp = b, *ap = a;
	
	while (totalBytesRead < totalBytes) {
		unsigned int bytesRemaining = totalBytes - totalBytesRead;
		unsigned int rowIndex = totalBytesRead / bytesPerRow;
		unsigned int rowByteIndex = totalBytesRead % bytesPerRow;
		unsigned int columnIndex = rowByteIndex / 4;
		unsigned int bytesToRead =
			(PIXEL_BUFFER_SIZE < bytesRemaining) ? PIXEL_BUFFER_SIZE : bytesRemaining;
		int bytesExtracted = 0;
		
		// NOTE: This stuff is necessary because we must avoid ending the read in the middle
		// of a pixel. If the buffer size was a multiple of 4, this would be redundant,
		// but it's more robust to include it.
		unsigned int newTotalBytesRead = totalBytesRead + bytesToRead;
		unsigned int newRowByteIndex = newTotalBytesRead % bytesPerRow;
		bytesToRead -= newRowByteIndex % 4; // Read a multiple of 4 bytes of the row.
		
		int bytesRead = std::fread(buffer, 1, bytesToRead, fptr);
		if (bytesRead != bytesToRead) {
			result = CGRESULT_READ_ERROR;
			goto finish;
		}
		
		while (bytesExtracted < bytesRead) {
			while (columnIndex < width && bytesExtracted < bytesRead) {
				unsigned int pixel = 0;
				std::memcpy(&pixel, buffer + bytesExtracted, 4);
				// NOTE: The extractors increment the output buffer pointers as necessary.
				rx(pixel, rp);
				gx(pixel, gp);
				bx(pixel, bp);
				ax(pixel, ap);
				columnIndex++;
				bytesExtracted += 4;
			}
			
			rowIndex++;
			columnIndex = 0;
		}
		
		totalBytesRead += bytesRead;
	}
	
	result = CGRESULT_OK;
	
finish:
	delete[] buffer;
	return result;
}

int readBMP(
	FILE *fptr, int dataFormat, int maxWidth, int maxHeight,
	int &width, int &height, void *r, void *g, void *b, void *a,
	int &result)
{
	unsigned int field = 0;
	int fieldsRead;
	
	// Read file header.
	fieldsRead = std::fread(&field, 2, 1, fptr); // 0: Read "BM".
	if (fieldsRead != 1 || field != 0x4d42U)
		return result = CGRESULT_INVALID_FORMAT;
	
	unsigned int fileSize = 0;
	fieldsRead = std::fread(&fileSize, 4, 1, fptr); // 2: Read file size.
	if (fieldsRead != 1)
		return result = CGRESULT_INVALID_FORMAT;
	
	fieldsRead = std::fread(&field, 4, 1, fptr); // 6: Read reserved fields.
	if (fieldsRead != 1)
		return result = CGRESULT_INVALID_FORMAT;
	
	unsigned int bitmapOffset = 0;
	fieldsRead = std::fread(&bitmapOffset, 4, 1, fptr); // 10: Read file offset to bitmap array.
	if (fieldsRead != 1)
		return result = CGRESULT_INVALID_FORMAT;
	
	// Read DIB header.
	unsigned int dibHeaderSize = 0;
	fieldsRead = std::fread(&dibHeaderSize, 4, 1, fptr); // 14: Read DIB header size.
	if (fieldsRead != 1 || dibHeaderSize != 40)
		return result = CGRESULT_UNSUPPORTED_FORMAT;
	
	width = 0;
	fieldsRead = std::fread(&width, 4, 1, fptr); // 18: Read image width.
	if (fieldsRead != 1 || width <= 0)
		return result = CGRESULT_UNSUPPORTED_FORMAT;
	if (width > maxWidth)
		return result = CGRESULT_BAD_DIMENSION;
	
	height = 0;
	fieldsRead = std::fread(&height, 4, 1, fptr); // 22: Read image height.
	if (fieldsRead != 1 || height <= 0)
		return result = CGRESULT_UNSUPPORTED_FORMAT;
	if (height > maxHeight)
		return result = CGRESULT_BAD_DIMENSION;
	
	fieldsRead = std::fread(&field, 2, 1, fptr); // 26: Read "number of color planes".
	if (fieldsRead != 1)
		return result = CGRESULT_UNSUPPORTED_FORMAT;
	
	unsigned int bitsPerPixel = 0;
	fieldsRead = std::fread(&bitsPerPixel, 2, 1, fptr); // 28: Read bits per pixel.
	if (fieldsRead != 1 || bitsPerPixel != 24 && bitsPerPixel != 32)
		return result = CGRESULT_UNSUPPORTED_FORMAT;
	unsigned int bytesPerPixel = bitsPerPixel / 8;
	
	unsigned int compressionMethod = 999999;
	fieldsRead = std::fread(&compressionMethod, 4, 1, fptr); // 30: Read compression method.
	if (fieldsRead != 1 || compressionMethod != 0) // BI_RGB
		return result = CGRESULT_UNSUPPORTED_FORMAT;
	
	unsigned int bitmapSize = 0;
	fieldsRead = std::fread(&bitmapSize, 4, 1, fptr); // 34: Read bitmap size.
	if (fieldsRead != 1)
		return result = CGRESULT_UNSUPPORTED_FORMAT;
	
	fieldsRead = std::fread(&field, 4, 1, fptr); // 38: Read horizontal resolution.
	if (fieldsRead != 1)
		return result = CGRESULT_UNSUPPORTED_FORMAT;
	
	fieldsRead = std::fread(&field, 4, 1, fptr); // 42: Read vertical resolution.
	if (fieldsRead != 1)
		return result = CGRESULT_UNSUPPORTED_FORMAT;
	
	fieldsRead = std::fread(&field, 4, 1, fptr); // 46: Read palette size.
	if (fieldsRead != 1)
		return result = CGRESULT_UNSUPPORTED_FORMAT;
	
	fieldsRead = std::fread(&field, 4, 1, fptr); // 50: Read "number of important colors".
	if (fieldsRead != 1)
		return result = CGRESULT_UNSUPPORTED_FORMAT;
	
	// 54: End of Headers.
	
	// Read pixel data.
	if (bitmapOffset > 54) {
		int fseekResult = std::fseek(fptr, bitmapOffset - 54, SEEK_CUR);
		if (fseekResult)
			return result = CGRESULT_SEEK_ERROR;
	}
	
	Extractor rx, gx, bx, ax;
	
	switch (dataFormat) {
	case CG_DATA_FORMAT_RGB:
		rx = redExtractor;
		gx = greenExtractor;
		bx = blueExtractor;
		ax = alphaExtractor;
		break;
	case CG_DATA_FORMAT_HCL:
		rx = hueExtractor;
		gx = chromaExtractor;
		bx = lumaExtractor;
		ax = alphaExtractor;
		break;
	case CG_DATA_FORMAT_RGB_BYTES:
		rx = redByteExtractor;
		gx = greenByteExtractor;
		bx = blueByteExtractor;
		ax = alphaByteExtractor;
		break;
	case CG_DATA_FORMAT_HCL_BYTES:
		rx = hueByteExtractor;
		gx = chromaByteExtractor;
		bx = lumaByteExtractor;
		ax = alphaByteExtractor;
		break;
	default:
		return result = CGRESULT_INVALID_ARGUMENT;
	}
	
	if (!r) { rx = nopExtractor; }
	if (!g) { gx = nopExtractor; }
	if (!b) { bx = nopExtractor; }
	if (!a) { ax = nopExtractor; }
	
	switch (bytesPerPixel) {
	case 3:
		read24bitPixels(fptr, width, height, rx, gx, bx, ax, r, g, b, a, result);
		break;
	case 4:
		read32bitPixels(fptr, width, height, rx, gx, bx, ax, r, g, b, a, result);
		break;
	default:
		return result = CGRESULT_UNSPECIFIED;
	}
	
	return result;
}


// Image File Write Access
int write24bitPixels(
	FILE *fptr, int width, int height, Packer3 packer,
	const void *r, const void *g, const void *b,
	int &result)
{
	char *buffer = new char[PIXEL_BUFFER_SIZE];
	if (!buffer)
		return result = CGRESULT_ALLOC_FAILED;
	
	unsigned int pixelBytesPerRow = 3 * width;
	unsigned int padBytesPerRow = (pixelBytesPerRow % 4 == 0) ? 0 : 4 - pixelBytesPerRow % 4;
	unsigned int bytesPerRow = pixelBytesPerRow + padBytesPerRow;
	unsigned int totalBytes = bytesPerRow * height;
	unsigned int totalBytesWritten = 0;
	const void *rp = r, *gp = g, *bp = b;
	
	while (totalBytesWritten < totalBytes) {
		unsigned int bytesRemaining = totalBytes - totalBytesWritten;
		unsigned int maxBytesBuffered =
			(PIXEL_BUFFER_SIZE-2 < bytesRemaining) ? PIXEL_BUFFER_SIZE-2 : bytesRemaining;
		unsigned int rowIndex = totalBytesWritten / bytesPerRow;
		unsigned int columnIndex = (totalBytesWritten % bytesPerRow) / 3;
		int bytesBuffered = 0;
		
		while (bytesBuffered < maxBytesBuffered) {
			while (columnIndex < width && bytesBuffered < maxBytesBuffered) {
				// NOTE: The packer increments the input buffer pointers as necessary.
				unsigned int pixel = packer(rp, gp, bp);
				std::memcpy(buffer + bytesBuffered, &pixel, 3);
				columnIndex++;
				bytesBuffered += 3;
			}
			
			if (padBytesPerRow > 0 && bytesBuffered < maxBytesBuffered) {
				std::memset(buffer + bytesBuffered, 0, padBytesPerRow);
				bytesBuffered += padBytesPerRow;
			}
			
			rowIndex++;
			columnIndex = 0;
		}
		
		int bytesWritten = std::fwrite(buffer, 1, bytesBuffered, fptr);
		if (bytesWritten != bytesBuffered) {
			result = CGRESULT_WRITE_ERROR;
			goto finish;
		}
		totalBytesWritten += bytesWritten;
	}
	
	result = CGRESULT_OK;
	
finish:
	delete[] buffer;
	return result;
}

int write32bitPixels(
	FILE *fptr, int width, int height, Packer4 packer,
	const void *r, const void *g, const void *b, const void *a,
	int &result)
{
	char *buffer = new char[PIXEL_BUFFER_SIZE];
	if (!buffer)
		return result = CGRESULT_ALLOC_FAILED;
	
	unsigned int pixelBytesPerRow = 4 * width;
	unsigned int bytesPerRow = pixelBytesPerRow;
	unsigned int totalBytes = bytesPerRow * height;
	unsigned int totalBytesWritten = 0;
	const void *rp = r, *gp = g, *bp = b, *ap = a;
	
	while (totalBytesWritten < totalBytes) {
		unsigned int bytesRemaining = totalBytes - totalBytesWritten;
		unsigned int maxBytesBuffered =
			(PIXEL_BUFFER_SIZE-3 < bytesRemaining) ? PIXEL_BUFFER_SIZE-3 : bytesRemaining;
		unsigned int rowIndex = totalBytesWritten / bytesPerRow;
		unsigned int columnIndex = (totalBytesWritten % bytesPerRow) / 4;
		int bytesBuffered = 0;
		
		while (bytesBuffered < maxBytesBuffered) {
			while (columnIndex < width && bytesBuffered < maxBytesBuffered) {
				// NOTE: The packer increments the input buffer pointers as necessary.
				unsigned int pixel = packer(rp, gp, bp, ap);
				std::memcpy(buffer + bytesBuffered, &pixel, 4);
				columnIndex++;
				bytesBuffered += 4;
			}
			
			rowIndex++;
			columnIndex = 0;
		}
		
		int bytesWritten = std::fwrite(buffer, 1, bytesBuffered, fptr);
		if (bytesWritten != bytesBuffered) {
			result = CGRESULT_WRITE_ERROR;
			goto finish;
		}
		totalBytesWritten += bytesWritten;
	}
	
	result = CGRESULT_OK;
	
finish:
	delete[] buffer;
	return result;
}

// TODO: Enforce upper limits on width and height. The number of bytes in the bitmap
// must fit in an unsigned 32-bit integer.
int writeBMP(
	FILE *fptr, int dataFormat, int width, int height,
	const void *r, const void *g, const void *b, const void *a,
	int &result)
{
	int bytesPerPixel = (a) ? 4 : 3;
	unsigned int pixelBytesPerRow = bytesPerPixel * width;
	unsigned int padBytesPerRow = (pixelBytesPerRow % 4 == 0) ? 0 : 4 - pixelBytesPerRow % 4;
	unsigned int bitmapSize = (pixelBytesPerRow + padBytesPerRow) * height;
	
	int fieldsWritten;
	unsigned int tmp;
	signed int stmp;
	
	// Write the file header.
	fieldsWritten = std::fwrite("BM", 2, 1, fptr); // 0: Write "BM".
	if (fieldsWritten != 1)
		return result = CGRESULT_WRITE_ERROR;
	
	tmp = 54 + bitmapSize;
	fieldsWritten = std::fwrite(&tmp, 4, 1, fptr); // 2: Write file size.
	if (fieldsWritten != 1)
		return result = CGRESULT_WRITE_ERROR;
	
	tmp = 0;
	fieldsWritten = std::fwrite(&tmp, 4, 1, fptr); // 6: Write zeroed reserved fields.
	if (fieldsWritten != 1)
		return result = CGRESULT_WRITE_ERROR;
	
	tmp = 54;
	fieldsWritten = std::fwrite(&tmp, 4, 1, fptr); // 10: Write file offset to bitmap array.
	if (fieldsWritten != 1)
		return result = CGRESULT_WRITE_ERROR;
	
	// Write the DIB header (BITMAPINFOHEADER).
	tmp = 40;
	fieldsWritten = std::fwrite(&tmp, 4, 1, fptr); // 14: Write DIB header size.
	if (fieldsWritten != 1)
		return result = CGRESULT_WRITE_ERROR;
	
	fieldsWritten = std::fwrite(&width, 4, 1, fptr); // 18: Write image width.
	if (fieldsWritten != 1)
		return result = CGRESULT_WRITE_ERROR;
	
	fieldsWritten = std::fwrite(&height, 4, 1, fptr); // 22: Write image height.
	if (fieldsWritten != 1)
		return result = CGRESULT_WRITE_ERROR;
	
	tmp = 1;
	fieldsWritten = std::fwrite(&tmp, 2, 1, fptr); // 26: Write "number of color planes".
	if (fieldsWritten != 1)
		return result = CGRESULT_WRITE_ERROR;
	
	tmp = 8 * bytesPerPixel;
	fieldsWritten = std::fwrite(&tmp, 2, 1, fptr); // 28: Write bits per pixel.
	if (fieldsWritten != 1)
		return result = CGRESULT_WRITE_ERROR;
	
	tmp = 0; // BI_RGB
	fieldsWritten = std::fwrite(&tmp, 4, 1, fptr); // 30: Write compression method (i.e. none).
	if (fieldsWritten != 1)
		return result = CGRESULT_WRITE_ERROR;
	
	fieldsWritten = std::fwrite(&bitmapSize, 4, 1, fptr); // 34: Write bitmap size.
	if (fieldsWritten != 1)
		return result = CGRESULT_WRITE_ERROR;
	
	tmp = 4000;
	fieldsWritten = std::fwrite(&tmp, 4, 1, fptr); // 38: Write horizontal resolution (pixels/m).
	if (fieldsWritten != 1)
		return result = CGRESULT_WRITE_ERROR;
	
	tmp = 4000;
	fieldsWritten = std::fwrite(&tmp, 4, 1, fptr); // 42: Write vertical resolution (pixels/m).
	if (fieldsWritten != 1)
		return result = CGRESULT_WRITE_ERROR;
	
	tmp = 0;
	fieldsWritten = std::fwrite(&tmp, 4, 1, fptr); // 46: Write palette size (no palette).
	if (fieldsWritten != 1)
		return result = CGRESULT_WRITE_ERROR;
	
	tmp = 0;
	fieldsWritten = std::fwrite(&tmp, 4, 1, fptr); // 50: Write "number of important colors".
	if (fieldsWritten != 1)
		return result = CGRESULT_WRITE_ERROR;
	
	// 54: End of Headers.
	
	// Write pixel data.
	Packer3 p3;
	Packer4 p4;
	
	switch (dataFormat) {
	case CG_DATA_FORMAT_RGB:
		p3 = packRGB;
		p4 = packRGBA;
		break;
	case CG_DATA_FORMAT_HCL:
		p3 = packHCLinRGB;
		p4 = packHCLAinRGBA;
		break;
	case CG_DATA_FORMAT_RGB_BYTES:
		p3 = packBytesRGB;
		p4 = packBytesRGBA;
		break;
	case CG_DATA_FORMAT_HCL_BYTES:
		p3 = packBytesHCLinRGB;
		p4 = packBytesHCLAinRGBA;
		break;
	default:
		return result = CGRESULT_INVALID_ARGUMENT;
	}
	
	switch (bytesPerPixel) {
	case 3:
		write24bitPixels(fptr, width, height, p3, r, g, b, result);
		break;
	case 4:
		write32bitPixels(fptr, width, height, p4, r, g, b, a, result);
		break;
	default:
		return result = CGRESULT_UNSPECIFIED;
	}
	
	return result;
}

int readImage(
	const std::string &path, const std::string &type, int dataFormat, int maxWidth, int maxHeight,
	int &width, int &height, void *r, void *g, void *b, void *a,
	int &result)
{
	int imageFormat = getImageFormat(path, type);
	if (imageFormat == CG_FILE_FORMAT_NONE)
		return result = CGRESULT_UNSUPPORTED_FORMAT;
	
	// Open the source file.
	FILE *fptr = std::fopen(path.c_str(), "rb");
	if (!fptr)
		return result = CGRESULT_FOPEN_FAILED;
	
	// Read the source file.
	result = CGRESULT_OK;
	
	switch (imageFormat) {
	case CG_FILE_FORMAT_BMP:
		readBMP(fptr, dataFormat, maxWidth, maxHeight, width, height, r, g, b, a, result);
		break;
	default:
		result = CGRESULT_UNSPECIFIED;
	}
	
	// Close the source file.
	int closeResult = std::fclose(fptr);
	if (closeResult)
		result = CGRESULT_FCLOSE_FAILED;
	
	return result;
}

int writeImage(
	const std::string &path, const std::string &type, int dataFormat, int width, int height,
	const void *r, const void *g, const void *b, const void *a,
	int &result)
{
	int imageFormat = getImageFormat(path, type);
	if (imageFormat == CG_FILE_FORMAT_NONE)
		return result = CGRESULT_UNSUPPORTED_FORMAT;
	
	// Open the destination file.
	FILE *fptr = std::fopen(path.c_str(), "wb");
	if (!fptr)
		return result = CGRESULT_FOPEN_FAILED;
	
	// Write the destination file.
	result = CGRESULT_OK;
	
	switch (imageFormat) {
	case CG_FILE_FORMAT_BMP:
		writeBMP(fptr, dataFormat, width, height, r, g, b, a, result);
		break;
	default:
		result = CGRESULT_UNSPECIFIED;
	}
	
	// Close the destination file.
	int closeResult = std::fclose(fptr);
	if (closeResult)
		result = CGRESULT_FCLOSE_FAILED;
	
	return result;
}

} // end anonymous namespace


// Public Interface
void graphics_init(int *out_result) {
	// NOTE: Nothing to to here at present.
	*out_result = CGRESULT_OK;
}

void graphics_convertRGBtoHCL(
	const int *width, const int *height, const double *r, const double *g, const double *b,
	double *out_h, double *out_c, double *out_l,
	int *out_result)
{
	int totalPixels = *width * *height;
	for (int i = 0; i < totalPixels; i++)
		convertRGBtoHCL(r[i], g[i], b[i], out_h[i], out_c[i], out_l[i]);
	*out_result = CGRESULT_OK;
}

void graphics_convertHCLtoRGB(
	const int *width, const int *height, const double *h, const double *c, const double *l,
	double *out_r, double *out_g, double *out_b,
	int *out_result)
{
	int totalPixels = *width * *height;
	for (int i = 0; i < totalPixels; i++)
		convertHCLtoRGB(h[i], c[i], l[i], out_r[i], out_g[i], out_b[i]);
	*out_result = CGRESULT_OK;
}

void graphics_readImageRGB(
	const char *file_name, const char *file_type, const int *max_width, const int *max_height,
	int *out_width, int *out_height, double *out_r, double *out_g, double *out_b, double *out_a,
	int *out_result)
{
	readImage(
		file_name, file_type, CG_DATA_FORMAT_RGB, *max_width, *max_height,
		*out_width, *out_height, out_r, out_g, out_b, out_a,
		*out_result);
}

void graphics_writeImageRGB(
	const char *file_name, const char *file_type, const int *width, const int *height,
	const double *r, const double *g, const double *b, const double *a,
	int *out_result)
{
	writeImage(
		file_name, file_type, CG_DATA_FORMAT_RGB, *width, *height,
		r, g, b, a,
		*out_result);
}

void graphics_readImageHCL(
	const char *file_name, const char *file_type, const int *max_width, const int *max_height,
	int *out_width, int *out_height, double *out_h, double *out_c, double *out_l, double *out_a,
	int *out_result)
{
	readImage(
		file_name, file_type, CG_DATA_FORMAT_HCL, *max_width, *max_height,
		*out_width, *out_height, out_h, out_c, out_l, out_a,
		*out_result);
}

void graphics_writeImageHCL(
	const char *file_name, const char *file_type, const int *width, const int *height,
	const double *h, const double *c, const double *l, const double *a,
	int *out_result)
{
	writeImage(
		file_name, file_type, CG_DATA_FORMAT_HCL, *width, *height,
		h, c, l, a,
		*out_result);
}

void graphics_convertBytesRGBtoHCL(
	const int *width, const int *height, const uchar *r, const uchar *g, const uchar *b,
	uchar *out_h, uchar *out_c, uchar *out_l,
	int *out_result)
{
	int totalPixels = *width * *height;
	for (int i = 0; i < totalPixels; i++) {
		double rd = doubleFrom8bit(r[i]);
		double gd = doubleFrom8bit(g[i]);
		double bd = doubleFrom8bit(b[i]);
		double hd, cd, ld;
		convertRGBtoHCL(rd, gd, bd, hd, cd, ld);
		out_h[i] = (uchar)doubleTo8bit(hd / 6.0);
		out_c[i] = (uchar)doubleTo8bit(cd);
		out_l[i] = (uchar)doubleTo8bit(ld);
	}
	*out_result = CGRESULT_OK;
}

void graphics_convertBytesHCLtoRGB(
	const int *width, const int *height, const uchar *h, const uchar *c, const uchar *l,
	uchar *out_r, uchar *out_g, uchar *out_b,
	int *out_result)
{
	int totalPixels = *width * *height;
	for (int i = 0; i < totalPixels; i++) {
		double hd = 6.0 * doubleFrom8bit(h[i]);
		double cd =       doubleFrom8bit(c[i]);
		double ld =       doubleFrom8bit(l[i]);
		double rd, gd, bd;
		convertHCLtoRGB(hd, cd, ld, rd, gd, bd);
		out_r[i] = (uchar)doubleTo8bit(rd);
		out_g[i] = (uchar)doubleTo8bit(gd);
		out_b[i] = (uchar)doubleTo8bit(bd);
	}
	*out_result = CGRESULT_OK;
}

void graphics_readImageBytesRGB(
	const char *file_name, const char *file_type, const int *max_width, const int *max_height,
	int *out_width, int *out_height, uchar *out_r, uchar *out_g, uchar *out_b, uchar *out_a,
	int *out_result)
{
	readImage(
		file_name, file_type, CG_DATA_FORMAT_RGB_BYTES, *max_width, *max_height,
		*out_width, *out_height, out_r, out_g, out_b, out_a,
		*out_result);
}

void graphics_writeImageBytesRGB(
	const char *file_name, const char *file_type, const int *width, const int *height,
	const uchar *r, const uchar *g, const uchar *b, const uchar *a,
	int *out_result)
{
	writeImage(
		file_name, file_type, CG_DATA_FORMAT_RGB_BYTES, *width, *height,
		r, g, b, a,
		*out_result);
}

void graphics_readImageBytesHCL(
	const char *file_name, const char *file_type, const int *max_width, const int *max_height,
	int *out_width, int *out_height, uchar *out_h, uchar *out_c, uchar *out_l, uchar *out_a,
	int *out_result)
{
	readImage(
		file_name, file_type, CG_DATA_FORMAT_HCL_BYTES, *max_width, *max_height,
		*out_width, *out_height, out_h, out_c, out_l, out_a,
		*out_result);
}

void graphics_writeImageBytesHCL(
	const char *file_name, const char *file_type, const int *width, const int *height,
	const uchar *h, const uchar *c, const uchar *l, const uchar *a,
	int *out_result)
{
	writeImage(
		file_name, file_type, CG_DATA_FORMAT_HCL_BYTES, *width, *height,
		h, c, l, a,
		*out_result);
}

void graphics_shutdown(int *out_result) {
	// NOTE: Nothing to to here at present.
	*out_result = CGRESULT_OK;
}
