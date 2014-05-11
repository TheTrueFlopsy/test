#ifndef CG_IMGDIFF_HPP
#define CG_IMGDIFF_HPP

int imageDiff(
	int nPixels,
	const double *i1_r, const double *i1_g, const double *i1_b,
	const double *i2_r, const double *i2_g, const double *i2_b,
	double *o_r, double *o_g, double *o_b);

#endif
