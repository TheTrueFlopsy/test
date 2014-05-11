
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
#include <iostream>
#include <iomanip>
#include "graphdll.hpp"
#include "leveleq.hpp"

const double SQRT2 = std::sqrt(2.0);

int main(int argc, const char **argv) {
	int w = 850;
	int h = 850;
	int size = w*h;
	/*double *r = new double[size];
	double *g = new double[size];
	double *b = new double[size];*/
	uchar *r = new uchar[size];
	uchar *g = new uchar[size];
	uchar *b = new uchar[size];
	
	/*double *rp = r;
	double *gp = g;
	double *bp = b;
	
	double diagE = std::sqrt((double)(w*w + h*h));
	double diagM = (double)(w+h);
	
	for (int y = 0; y < h; y++) {
		for (int x = 0; x < h; x++) {
			*rp++ = std::sqrt((double)(x*x + y*y))/diagE;
			//*rp++ = 0.0;
			*gp++ = (double)((w-x)+y)/diagM;
			//*gp++ = 0.0;
			*bp++ = (double)(x+(h-y))/diagM;
			//*bp++ = 0.0;
		}
	}*/
	
	int result;
	int w1 = 0;
	int h1 = 0;
	
	// Rotate RGB channels.
	/*graphics_readImageRGB(argv[1], "bmp", &w, &h, &w1, &h1, r, g, b, 0, &result);
	if (!result)
		graphics_writeImageRGB(argv[2], "bmp", &w1, &h1, g, b, r, 0, &result);*/
	
	// Output HCL luma channel as grayscale image.
	/*graphics_readImageHCL(argv[1], "bmp", &w, &h, &w1, &h1, r, g, b, 0, &result);
	if (!result)
		graphics_writeImageRGB(argv[2], "bmp", &w1, &h1, b, b, b, 0, &result);*/
	
	// Convert from RGB to HCL and back again.
	/*graphics_readImageHCL(argv[1], "bmp", &w, &h, &w1, &h1, r, g, b, 0, &result);
	if (!result)
		graphics_writeImageHCL(argv[2], "bmp", &w1, &h1, r, g, b, 0, &result);*/
	
	// Rotate RGB byte channels.
	/*graphics_readImageBytesRGB(argv[1], "bmp", &w, &h, &w1, &h1, r, g, b, 0, &result);
	if (!result)
		graphics_writeImageBytesRGB(argv[2], "bmp", &w1, &h1, g, b, r, 0, &result);*/
	
	// Convert from RGB to HCL bytes and back again.
	/*graphics_readImageBytesHCL(argv[1], "bmp", &w, &h, &w1, &h1, r, g, b, 0, &result);
	if (!result)
		graphics_writeImageBytesHCL(argv[2], "bmp", &w1, &h1, r, g, b, 0, &result);*/
	
	// Convert from RGB to HCL bytes, equalize luma levels and convert back again.
	graphics_readImageBytesHCL(argv[1], "bmp", &w, &h, &w1, &h1, r, g, b, 0, &result);
	if (!result) {
		equalizeLevels(w1*h1, b);
		graphics_writeImageBytesHCL(argv[2], "bmp", &w1, &h1, r, g, b, 0, &result);
	}
	
	delete[] r;
	delete[] g;
	delete[] b;
	
	std::cout << "result=" << result << std::endl;
	
	return 0;
}
