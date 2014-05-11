
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
#include "graphdll.hpp"
#include "imgdiff.hpp"

namespace {

const double SQRT3 = std::sqrt(3.0);

double pixelDistance(double r1, double g1, double b1, double r2, double g2, double b2) {
	double dr = r2 - r1;
	double dg = g2 - g1;
	double db = b2 - b1;
	return std::sqrt(dr*dr + dg*dg + db*db) / SQRT3;
}

}

int imageDiff(
	int nPixels,
	const double *i1_r, const double *i1_g, const double *i1_b,
	const double *i2_r, const double *i2_g, const double *i2_b,
	double *o_r, double *o_g, double *o_b)
{
	int height = 1, res = -9999;
	//double *i1_h = new double[nPixels];
	//double *i1_c = new double[nPixels];
	double *i1_l = new double[nPixels];
	//double *i2_h = new double[nPixels];
	//double *i2_c = new double[nPixels];
	double *i2_l = new double[nPixels];
	//graphics_convertRGBtoHCL(&nPixels, &height, i1_r, i1_g, i1_b, i1_h, i1_c, i1_l, &res);
	//graphics_convertRGBtoHCL(&nPixels, &height, i2_r, i2_g, i2_b, i2_h, i2_c, i2_l, &res);
	graphics_convertRGBtoHCL(&nPixels, &height, i1_r, i1_g, i1_b, i1_l, i1_l, i1_l, &res);
	graphics_convertRGBtoHCL(&nPixels, &height, i2_r, i2_g, i2_b, i2_l, i2_l, i2_l, &res);
	
	for (int k = 0; k < nPixels; k++) {
		o_r[k] = i2_l[k];
		o_g[k] = i1_l[k];
		
		//i1_h[k] = 0.0;
		//i1_c[k] = 0.0;
		i1_l[k] = 0.5 * (i1_l[k] + i2_l[k]);
		
		double d  = pixelDistance(i1_r[k], i1_g[k], i1_b[k], i2_r[k], i2_g[k], i2_b[k]);
		double md =  1.0 - d;
		
		double gray    = md * i1_l[k];
		double magenta =  d *  o_r[k];
		double green   =  d *  o_g[k];
		
		o_r[k] = gray + magenta;
		o_g[k] = gray + green;
		o_b[k] = o_r[k];
	}
	
	delete[] i1_l;
	delete[] i2_l;
	
	/*double *i1_r2 = i1_h;
	double *i1_g2 = i1_c;
	double *i1_b2 = i1_l;
	double *i2_r2 = i2_h;
	double *i2_g2 = i2_c;
	double *i2_b2 = i2_l;
	graphics_convertHCLtoRGB(&nPixels, &height, i1_h, i1_c, i1_l, i1_r2, i1_g2, i1_b2, &res);
	graphics_convertHCLtoRGB(&nPixels, &height, i2_h, i2_c, i2_l, i2_r2, i2_g2, i2_b2, &res);
	i1_h = 0;
	i1_c = 0;
	i1_l = 0;
	i2_h = 0;
	i2_c = 0;
	i2_l = 0;
	
	for (int k = 0; k < nPixels; k++) {
		double d = pixelDistance(i1_r[k], i1_g[k], i1_b[k], i2_r[k], i2_g[k], i2_b[k]);
		double md =  1.0 - d;
		
		o_r[k] = md*i1_r2[k] + d*o_r[k];
		o_g[k] = md*i1_g2[k] + d*o_g[k];
		o_b[k] = md*i1_b2[k];
	}
	
	delete[] i1_r2;
	delete[] i1_g2;
	delete[] i1_b2;
	delete[] i2_r2;
	delete[] i2_g2;
	delete[] i2_b2;*/
	
	return 0;
}
