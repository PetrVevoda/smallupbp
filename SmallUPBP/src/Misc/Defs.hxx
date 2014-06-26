/*
 * Copyright (C) 2014, Petr Vevoda, Martin Sik (http://cgg.mff.cuni.cz/~sik/), 
 * Tomas Davidovic (http://www.davidovic.cz), Iliyan Georgiev (http://www.iliyan.com/), 
 * Jaroslav Krivanek (http://cgg.mff.cuni.cz/~jaroslav/)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * (The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
 */

#ifdef NULL
    #undef NULL
#endif

#define NULL nullptr


#ifndef ___DEFS_HXX___
#define ___DEFS_HXX___


#include <limits>
#include <iostream>


//////////////////////////////////////////////////////////////////////////
// Typedefs

typedef __int8              int8;
typedef __int16             int16;
typedef __int64             int64;
typedef unsigned int        uint;
typedef unsigned __int8     uint8;
typedef unsigned __int16    uint16;
typedef unsigned __int64    uint64;


//////////////////////////////////////////////////////////////////////////
// Constants

//! An epsilon value (small non-zero positive number). Used in BSDF computations.
const float EPS_COSINE = 1e-6f;

//! An epsilon value (small non-zero positive number). Used in BSDF computations.
const float EPS_PHONG = 1e-3f;

//! An epsilon value (small non-zero positive number). Used in ray vs geometry intersection tests.
const float EPS_RAY    = 1e-3f;

//! Floating point Pi (Ludolf number).
const float PI_F = 3.141592653589793238462643383279f;

//! Floating point inverse of Pi (Ludolf number).
const float INV_PI_F = (1.f / PI_F);

//! Floating point positive infinity (very large number).
const float INFINITY = 1e36f;

//! Floating point positive infinity (from STL numeric limits).
const float FLOAT_INFINITY = std::numeric_limits<float>::infinity();


//////////////////////////////////////////////////////////////////////////
// Defs

#if 1

/**
 * @brief	If assertion fails, outputs its file name and line number and terminates.
 *
 * @param	expr	The expression.
 */
#define UPBP_ASSERT(expr) \
	do { if(!(expr)) { std::cerr << "Error: assertion `"#expr"' failed at " << __FILE__ << ":" << __LINE__ << std::endl; exit(2); } } while(0)
#else

/**
 * @brief	No action.
 *
 * @param	expr	The expression.
 */
#define UPBP_ASSERT(expr)
#endif

/**
 * @brief	Forced inlining of a function - because other compilers like g++ have different
 * 			forced inline directives.
 */
#define INLINE __forceinline

/**
 * @brief	Disables SSE 4.0/4.1 intrinsics (to be able to run on older CPUs). Uncommenting
 * 			produces a legacy build. Legacy and full-speed builds are otherwise binary-compatible.
 */
//#define LEGACY_CPU

#ifndef likely
#if defined(_MSC_VER) && !defined(__INTEL_COMPILER)
#define   likely(expr) expr
#define unlikely(expr) expr
#else
#define   likely(expr) __builtin_expect(expr,true )
#define unlikely(expr) __builtin_expect(expr,false)
#endif
#endif


//////////////////////////////////////////////////////////////////////////
// Enums

/**
 * @brief	Type of geometry used in a scene.
 */
enum GeometryType
{
	REAL = 0,     //!< Ordinary geometry.
	IMAGINARY = 1 //!< Geometry with no surface used only as containers for media.
};

/**
 * @brief	Type of a beam used either for a query or a stored photon beam.
 */
enum BeamType
{
	SHORT_BEAM = 1, //!< Terminates at scattering in media.
	LONG_BEAM = 2   //!< Continues to a surface hit.
};

/**
 * @brief	How we compute photon radius.
 */
enum RadiusCalculation
{
	CONSTANT_RADIUS = 0,  //!< Constant radius for all photons in iteration.
	KNN_RADIUS = 1        //!< Radius computed according to surrounding photons.
};

/**
 * @brief	Type of reduction of a number of photon beams stored within a grid cell.
 */
enum BeamReduction
{
	PRESAMPLE = 0,      //!< Stored: all beams randomly shuffled, tested: first fixed number of stored beams.
	OFFSET = 1,         //!< Stored: all beams randomly shuffled, tested: fixed number of stored beams beginning at a random offset.
	RESAMPLE_FIXED = 2, //!< Stored: all beams, tested: fixed number of randomly picked stored beams.
	RESAMPLE = 3	    //!< Stored: all beams, tested: all stored beams which were accepted in a random test.
};

/**
 * @brief	Estimators available to combine in the UPBP renderer.
 */
enum EstimatorTechnique
{
	BPT = 4,	//!< Bidirectional path tracer.
	SURF = 8,   //!< Surface photon mapping.
	PP3D = 16,  //!< Medium photon mapping (point vs points).
	PB2D = 32,  //!< BRE (beam vs points).
	BB1D = 64   //!< Photon beams (beam vs beams).
};

/**
 * @brief	Misc flags.
 */
enum OtherSettings
{
	NO_SINE_IN_WEIGHTS = 128, //!< Do not use the sine factor in a MIS weight for the BB1D estimator.
	PREVIOUS = 256,           //!< Run all used estimators in a previous mode.
	COMPATIBLE = 512,         //!< Run all used estimators in a compatible mode.
	BB1D_PREVIOUS = 1024,     //!< Run the BB1D estimator in a previous mode.
	SPECULAR_ONLY = 2048,     //!< Render only specular paths.
};

#endif //__DEFS_HXX__