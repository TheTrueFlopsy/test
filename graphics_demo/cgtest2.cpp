
/*
Copyright (c) 2014, Johan Sarge
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
#include "imgdiff.hpp"

const double SQRT2 = std::sqrt(2.0);

int main(int argc, const char **argv) {
	int res = CGRESULT_UNSPECIFIED;
	int w = 850;
	int h = 850;
	int size = w*h;
	int w1, h1;
	double *r1 = new double[size];
	double *g1 = new double[size];
	double *b1 = new double[size];
	int w2, h2;
	double *r2 = new double[size];
	double *g2 = new double[size];
	double *b2 = new double[size];
	int w3, h3;
	double *r3 = new double[size];
	double *g3 = new double[size];
	double *b3 = new double[size];
	
	graphics_readImageRGB(argv[1], "bmp", &w, &h, &w1, &h1, r1, g1, b1, 0, &res);
	if (res != CGRESULT_OK)
		goto finish;
	
	graphics_readImageRGB(argv[2], "bmp", &w, &h, &w2, &h2, r2, g2, b2, 0, &res);
	if (res != CGRESULT_OK)
		goto finish;
	
	if (w1 != w2 || h1 != h2) {
		res = CGRESULT_UNSPECIFIED + CGRESULT_BAD_DIMENSION;
		goto finish;
	}
	
	res = imageDiff(size, r1, g1, b1, r2, g2, b2, r3, g3, b3);
	if (res != CGRESULT_OK)
		goto finish;
	
	graphics_writeImageRGB(argv[3], "bmp", &w1, &h1, r3, g3, b3, 0, &res);
	
finish:	
	delete[] r1;
	delete[] g1;
	delete[] b1;
	delete[] r2;
	delete[] g2;
	delete[] b2;
	delete[] r3;
	delete[] g3;
	delete[] b3;
	
	std::cout << "result=" << res << std::endl;
	return res;
}
