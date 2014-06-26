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

#ifndef __MATERIALS_HXX__
#define __MATERIALS_HXX__

#include <vector>
#include <cmath>

#include "..\Path\Frame.hxx"
#include "..\Path\Ray.hxx"
#include "..\Misc\Utils2.hxx"

#define THIN_WALL_PRIORITY -1
#define GLOBAL_MEDIUM_PRIORITY -2

class Material
{
public:
    Material()
    {
        Reset();
    }

    void Reset()
    {
        mDiffuseReflectance = Rgb(0);
        mPhongReflectance   = Rgb(0);
        mPhongExponent      = 1.f;
        mMirrorReflectance  = Rgb(0);
        mIOR                = -1.f;
		mPriority			= THIN_WALL_PRIORITY;
		mGeometryType		= REAL;
    }

    // diffuse is simply added to the others
    Rgb mDiffuseReflectance;
    
	// Phong is simply added to the others
    Rgb mPhongReflectance;
    float mPhongExponent;

    // mirror can be either simply added, or mixed using Fresnel term
    // this is governed by mIOR, if it is >= 0, Fresnel is used, otherwise
    // it is not
    Rgb mMirrorReflectance;

    // When mIOR >= 0, we also transmit (just clear glass)
    float mIOR;

	int mPriority; // When mPriority == -1 -> it does not change medium

	GeometryType mGeometryType; // Real or imaginary

	/* 
	 * There are two types of materials: real and imaginary. Geometry with a real
	 * material acts as expected, depending on its properties incident ray gets reflected
	 * or refracted. Geometry with an imaginary material is only a container for medium.
	 * Ray does not interact with its boundary at all. Type of a material is specified
	 * by geometry type property. Another important material property is priority. It is
	 * necessary in case of overlapping geometry with different materials for decision
	 * which of them is in effect. Following rules must be obeyed:
	 *  - no imaginary material can have greater priority than a real material
	 *  - no object with an imaginary material can intersect an object with a real material (but it can include it completely)
	 *  - materials of intersecting objects cannot have same priorities  
	 */
};

#endif //__MATERIALS_HXX__
