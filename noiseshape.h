/*
* Copyright (C) 2016 - 2019 Judd Niemann - All Rights Reserved.
* You may use, distribute and modify this code under the
* terms of the GNU Lesser General Public License, version 2.1
*
* You should have received a copy of GNU Lesser General Public License v2.1
* with this file. If not, please refer to: https://github.com/jniemann66/ReSampler
*/

#ifndef NOISESHAPE_H
#define NOISESHAPE_H 1

// noiseshape.h : contains filter coefficients for noise shaping

//////////////////////////
//
// Filter Coefficients
//
//////////////////////////

namespace ReSampler {

const double noiseShaperPassThrough[1] = {
	1
};

// filters based on E-weighted curves
// from 'Minimally Audible Noise Shaping' (*)

const double modew44[9] = { // Modified E-weighted (appendix: 2)
	1.6620, -1.2630, 0.4827,
	-0.2913, 0.1268,-0.1124,
	0.03252, -0.01265, -0.03524
};

const double lips44[5] = { // improved E-weighted (appendix: 5)
	2.033, -2.165, 1.959,
	-1.590, 0.6149
};

const double impew44[9] = { // improved E-weighted 9 coeff (appendix: 6)
	2.847, -4.685,  6.214,
	-7.184, 6.639, -5.032,
	3.263, -1.632, 0.4191
};

// filters based on F-weighted curves
// from 'Psychoacoustically Optimal Noise Shaping' (**)
// this filter is the "F-Weighted" noise filter described by Wannamaker
// It is designed to produce minimum audibility:

const double wan3[3] = { // Table 3; 3 Coefficients
	1.623, -0.982, 0.109
};

const double wan9[9] = { // Table 3; 9 Coefficients ('f-weighted' in SoX)
	2.4120002321781  , -3.370000324394779,  3.937000378973959,
	-4.174000401787478,  3.353000322758366, -2.205000212252369,
	1.281000123308519, -0.569000054771701,  0.084700008153185
};

const double wan24[24] = { // Table 4; 24 Coefficients
	2.391510032751124, -3.284444044979632,  3.679506050389904,
	-3.635044049781009,  2.524185034568077, -1.146701015703782,
	0.115354001579743,  0.51374500703561 , -0.749277010261162,
	0.512386007016999, -0.188997002588268, -0.043705000598528,
	0.149843002052063, -0.151186002070453,  0.076302001044937,
	-0.012070000165296, -0.021127000289329,  0.025232000345547,
	-0.016121000220773,  0.004453000060982,  0.000876000011999,
	-0.001799000024635,  0.0007740000106  , -0.000128000001755
};

const double std_44[10] = {
	2.1458349310385, -2.8886247979953, 3.1440240984815,
	-2.7691389969061,  1.8410749156878, -1.03787966035156,
	0.45566921793679, -0.12410827054639,  0.013088865078523,
	-0.002197012454707
};

const double high28[13] = { // 11.25khz notch, 28dB High Shelf
	2.669515030552269, -4.611433627036229,  5.553683187474162,
	-5.131323713251633,  3.433819528157691, -1.653305160414548,
	0.371229110458052,  0.05145445300671 , -0.127107070478664,
	0.061701440817659, -0.05840987108654 ,  0.028532248319127,
	-0.010425978385254
};

const double high30[10] = { // 11.25khz notch, 30dB High Shelf
	2.805430627176799, -4.940936304404892,  6.221826870052102,
	-6.086670498094316,  4.486608520827646, -2.519447169758117,
	0.941337610113574, -0.21595651823809 , -0.005781498274259,
	0.002596637355373
};

const double high32[12] = { // 11.25khz notch, 32dB High Shelf
	3.165340739587537, -6.204765443967887,  8.711133333140808,
	-9.594907683918725,  8.359115029982675, -5.938493225620955,
	3.403895161842914, -1.681740133694475,  0.727810978096891,
	-0.308763921621752,  0.098112460564138, -0.020921651243242
};

const double blue[23] = { // approximation of "blueing" filter (turn white noise into blue 3dB/oct)
	0.585269620841384,  0.082071736897062,
	0.037899214901607,  0.029475690961127,  0.024598677570425,
	0.020691366467455,  0.017442308633106,  0.014730846261233,
	0.01246687963374 ,  0.010576070047764,  0.008996484088255,
	0.007676478017599,  0.006572984608519,  0.00565008804648 ,
	0.004877837034592,  0.004231253858972,  0.003689508152382,
	0.003235228516421,  0.002853927755039,  0.002533524334537,
	0.002263942227748,  0.002036778620016,  0.001845025142107
};

} // namespace ReSampler

/*
*	Note on deriving FIR coefficients to achieve a desired noise-shaping response:
*	The dithering topology includes a delay (Z^-1) in the feedback loop, 
*	and this needs to be factored into the design.
*	To use an externally-designed FIR filter with a given response,
*	normalize the FIR coefficients so that the first coefficient becomes -1.
*	Discard the first ( = -1) coefficient, and use the remaining coefficients.
*	Example:
*	from filter designer: 1.0148382 , -1.4075289 ,  1.136079  , -0.92514559,  0.52843289
*	Dividing by -1.0148382 and discarding the first coefficient, this becomes:
*	1.386949072275758, -1.119468108315197, 0.911618807806013, -0.52070654218574
*	(of course, when using coeffs intended for a  noise-shaper with similar topology, 
*	just use the coeffs as-is ...)
*	The FIR filter should be minimum-phase.
*/

// *Minimally Audible Noise Shaping
// STANLEY P. LIPSHITZ,JOHN VANDERKOOY, ROBERT A. WANNAMAKER
// J.AudioEng.Soc.,Vol.39,No.11,1991November

// **Psychoacoustically Optimal Noise Shaping
// Robert. A. Wannamaker
// Journal of the Audio Engineering Society 40(7 / 8) : 611 - 620 � July 1992

#endif
