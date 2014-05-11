
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

#include <iostream>
#include <iomanip>

#include <vector>
#include "leveleq.hpp"

namespace {

struct PixCounter {
	PixCounter() : pixcount(-1), pixval(0) {}
	PixCounter(int c, unsigned char v) : pixcount(c), pixval(v) {}
	int pixcount;
	unsigned char pixval;
};

}

int equalizeLevels(int nPixels, unsigned char *pixels) {
	int *srcpixcounts = new int[256];
	int *trgpixcounts = new int[256];
	std::vector<PixCounter> *srcpixvals = new std::vector<PixCounter>[256];
	
	std::cerr << "Counting source pixels." << std::endl;
	for (int v = 0; v < 256; v++)
		srcpixcounts[v] = 0;
	
	for (int i = 0; i < nPixels; i++)
		srcpixcounts[pixels[i]]++;
	
	int avgpixcount = nPixels / 256;
	int rempixcount = nPixels % 256;
	
	std::cerr << "Initializing target pixel counters." << std::endl;
	for (int v = 0; v < 256; v++)
		trgpixcounts[v] = avgpixcount + ((v < rempixcount) ? 1 : 0);
	
	std::cerr << "Mapping source pixel values to target values." << std::endl;
	int *trgpixcount = trgpixcounts;
	for (int src = 0; src < 256; src++) {
		int srcpixcount = srcpixcounts[src];
		
		while (*trgpixcount < srcpixcount) {
			srcpixvals[src].push_back(
				PixCounter(*trgpixcount, (unsigned char)(trgpixcount - trgpixcounts)));
			srcpixcount -= *trgpixcount;
			*trgpixcount++ = 0;
		}
		
		srcpixvals[src].push_back(
			PixCounter(srcpixcount, (unsigned char)(trgpixcount - trgpixcounts)));
		*trgpixcount -= srcpixcount;
		if (*trgpixcount == 0)
			trgpixcount++;
	}
	
	std::cerr << "Swapping pixel values." << std::endl;
	for (int i = 0; i < nPixels; i++) {
		int srcval = pixels[i];
		PixCounter &trgcounter = srcpixvals[srcval].back();
		pixels[i] = trgcounter.pixval;
		if (trgcounter.pixcount-- == 1) // Last pixel used.
			srcpixvals[srcval].pop_back();
	}
	
	std::cerr << "Cleaning up." << std::endl;
	delete[] srcpixcounts;
	delete[] trgpixcounts;
	delete[] srcpixvals;
	return 0;
}
