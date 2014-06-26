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

#ifndef __VLTPATHVERTEX_HXX__
#define __VLTPATHVERTEX_HXX__

#include "Bsdf.hxx"

// Volume light tracer path vertex, used for merging and connection
/**
   This path vertex correspond to a particle just before a collision.
   mHitpoint corresponds to the next collision site.
   mThroughput is the accumulated path throughput up to this point (including emission and division by path PDF).
   mPrevThroughput is the throughput before the transition through the media is applied, i.e. just after leaving the previous collision (or emission).
   mIncEdgeLength is the length traveled from the previous collision site to the current location.
 */
template<bool tFromLight>
struct VltPathVertex
{		
	Pos   mHitpoint;           //!< Position of the vertex
	Rgb   mThroughput;         //!< Path throughput (including emission and division by path PDF)
	Rgb   mPrevThroughput;     //!< Path throughput (including emission and division by path PDF)
	float mIncEdgeLength;      //!< Length of the incident path edge
	uint  mPathLength  : 30;   //!< Number of segments between source and vertex
	uint  mIsInMedium  :  1;   //!< True if this path vertex is in medium
	uint  mXXXXXXXXXX  :  1;   //!< Additional flag

	// Stores all required local information, including incoming direction.
	BSDF mBSDF;
};

typedef VltPathVertex<true>  VltLightVertex;

#endif //__VLTPATHVERTEX_HXX__
