
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

#ifndef CG_GRAPHDLL_HPP
#define CG_GRAPHDLL_HPP

#ifdef WIN32
#ifdef CG_GRAPHDLL_DLL_BUILD
#define CG_GRAPHDLL_DLL_EXPORT __attribute__ ((dllexport))
#else
#define CG_GRAPHDLL_DLL_EXPORT __attribute__ ((dllimport))
#endif
#else
#define CG_GRAPHDLL_DLL_EXPORT
#endif

typedef unsigned char uchar;

enum {
	CGRESULT_OK = 0,
	
	CGRESULT_INVALID_ARGUMENT   =  -2,
	CGRESULT_ALLOC_FAILED       =  -3,
	
	CGRESULT_FOPEN_FAILED       =  -4,
	CGRESULT_INVALID_FORMAT     =  -5,
	CGRESULT_UNSUPPORTED_FORMAT =  -6,
	CGRESULT_READ_ERROR         =  -7,
	CGRESULT_SEEK_ERROR         =  -8,
	CGRESULT_WRITE_ERROR        =  -9,
	CGRESULT_INCOMPLETE_READ    = -10,
	CGRESULT_INCOMPLETE_WRITE   = -11,
	CGRESULT_BAD_DIMENSION      = -12,
	CGRESULT_FCLOSE_FAILED      = -13,
	
	CGRESULT_UNSPECIFIED = -1000
};

enum {
	CG_FILE_FORMAT_NONE = 0,
	CG_FILE_FORMAT_RAW  = 1,
	CG_FILE_FORMAT_BMP  = 2
};

enum {
	CG_DATA_FORMAT_NONE      = 0,
	CG_DATA_FORMAT_RGB       = 1,
	CG_DATA_FORMAT_HCL       = 2,
	CG_DATA_FORMAT_RGB_BYTES = 3,
	CG_DATA_FORMAT_HCL_BYTES = 4
};

// NOTE: These functions assume that all channel buffers store values row-by-row, left-to-right
// and bottom-to-top. That is, the origin of the pixel coordinate system is at the lower left
// corner of the image and the channel values at coordinates (x,y) in an image of width W is
// stored at index (x + W*y) in the corresponding channel buffers.

extern "C" {

CG_GRAPHDLL_DLL_EXPORT
void graphics_init(int *out_result);

CG_GRAPHDLL_DLL_EXPORT
void graphics_convertRGBtoHCL(
	const int *width, const int *height, const double *r, const double *g, const double *b,
	double *out_h, double *out_c, double *out_l,
	int *out_result);

CG_GRAPHDLL_DLL_EXPORT
void graphics_convertHCLtoRGB(
	const int *width, const int *height, const double *h, const double *c, const double *l,
	double *out_r, double *out_g, double *out_b,
	int *out_result);

CG_GRAPHDLL_DLL_EXPORT
void graphics_readImageRGB(
	const char *file_name, const char *file_type, const int *max_width, const int *max_height,
	int *out_width, int *out_height, double *out_r, double *out_g, double *out_b, double *out_a,
	int *out_result);

CG_GRAPHDLL_DLL_EXPORT
void graphics_writeImageRGB(
	const char *file_name, const char *file_type, const int *width, const int *height,
	const double *r, const double *g, const double *b, const double *a,
	int *out_result);

CG_GRAPHDLL_DLL_EXPORT
void graphics_readImageHCL(
	const char *file_name, const char *file_type, const int *max_width, const int *max_height,
	int *out_width, int *out_height, double *out_h, double *out_c, double *out_l, double *out_a,
	int *out_result);

CG_GRAPHDLL_DLL_EXPORT
void graphics_writeImageHCL(
	const char *file_name, const char *file_type, const int *width, const int *height,
	const double *h, const double *c, const double *l, const double *a,
	int *out_result);

CG_GRAPHDLL_DLL_EXPORT
void graphics_convertBytesRGBtoHCL(
	const int *width, const int *height, const uchar *r, const uchar *g, const uchar *b,
	uchar *out_h, uchar *out_c, uchar *out_l,
	int *out_result);

CG_GRAPHDLL_DLL_EXPORT
void graphics_convertBytesHCLtoRGB(
	const int *width, const int *height, const uchar *h, const uchar *c, const uchar *l,
	uchar *out_r, uchar *out_g, uchar *out_b,
	int *out_result);

CG_GRAPHDLL_DLL_EXPORT
void graphics_readImageBytesRGB(
	const char *file_name, const char *file_type, const int *max_width, const int *max_height,
	int *out_width, int *out_height, uchar *out_r, uchar *out_g, uchar *out_b, uchar *out_a,
	int *out_result);

CG_GRAPHDLL_DLL_EXPORT
void graphics_writeImageBytesRGB(
	const char *file_name, const char *file_type, const int *width, const int *height,
	const uchar *r, const uchar *g, const uchar *b, const uchar *a,
	int *out_result);

CG_GRAPHDLL_DLL_EXPORT
void graphics_readImageBytesHCL(
	const char *file_name, const char *file_type, const int *max_width, const int *max_height,
	int *out_width, int *out_height, uchar *out_h, uchar *out_c, uchar *out_l, uchar *out_a,
	int *out_result);

CG_GRAPHDLL_DLL_EXPORT
void graphics_writeImageBytesHCL(
	const char *file_name, const char *file_type, const int *width, const int *height,
	const uchar *h, const uchar *c, const uchar *l, const uchar *a,
	int *out_result);

CG_GRAPHDLL_DLL_EXPORT
void graphics_shutdown(int *out_result);

}

#endif
