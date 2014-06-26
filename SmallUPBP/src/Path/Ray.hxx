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

#ifndef __RAY_HXX__
#define __RAY_HXX__

#include <vector>
#include <cmath>
#include <list>

#include "StaticArray.hxx"
#include "..\Structs\Rgb.hxx"

//////////////////////////////////////////////////////////////////////////
// Ray casting structures

namespace SmallUPBP
{
	// Class representing a ray (half-line with given origin point and direction).
	class Ray
	{
	public:
		// Point of the ray origin.
		Pos origin;

		// Ray direction.
		Dir direction;

		INLINE Ray() { }

		INLINE Ray(const Pos origin, const Dir direction) : origin(origin), direction(direction) { }

		// Returns a point on the ray with the given ray parameter.
		INLINE Pos target(const float t = 1.f) const {
			return origin + (direction*t);
		}
	};
}

typedef SmallUPBP::Ray Ray;

// Either an intersection of a ray and geometry or a place where scattering event was sampled while a ray was traveling through a medium.
struct Isect
{
public:
	
	Isect (float aMaxDist = INFINITY)
	{
		mDist = aMaxDist;
		mMatID = -1;
		mMedID = -1;
		mLightID = -1;
		mNormal = Dir(0);
		mEnter = false;
	}

	float mDist;          // Distance to the closest intersection on a ray (serves as ray.tmax).
	int   mMatID;         // ID of intersected material, -1 indicates scattering event inside medium.
	int   mMedID;         // ID of interacting medium or medium behind the hit surface, -1 means that crossing the hit surface does not affect medium.
	int   mLightID;       // ID of intersected light, -1 means none.
	Dir   mNormal;        // Normal at the intersection.
	bool  mEnter;         // Whether the ray enters geometry at this intersection (cosine of its direction and the normal is negative).
	int   mElementID;     // ID of intersected geometry element - used for calculating shading normal etc.
	Vec2f mUV;		      // Barycentric coordinates of a hitpoint.
	Dir   mShadingNormal; // Shading normal.

	// True if this Isect represents a scattering event inside a medium.
	bool IsInMedium() const
	{
		return mMatID < 0;
	}

	// True if this Isect represents an intersection with geometry.
	bool IsOnSurface() const
	{
		return mMatID >= 0;
	}

	// Whether properties of this Isect have valid values.
	bool IsValid() const
	{
		return mDist > 0 && ((mMatID < 0 && mMedID >= 0) || (mMatID >= 0 && mNormal != Dir(0.0f))) && (mLightID < 0 || mMatID >= 0);
	}

	// Enables sorting Isects according their distance on a ray.
	bool operator<(Isect right)
	{
		return mDist < right.mDist;
	}
};

struct VolumeSegment;
struct VolumeSegmentLite;

#define USE_STATIC_ARRAYS

#ifndef USE_STATIC_ARRAYS
typedef std::list<Isect> Intersections;
typedef std::vector<VolumeSegmentLite> LiteVolumeSegments;
typedef std::vector<VolumeSegment> VolumeSegments;
#else
typedef StaticArray<VolumeSegmentLite, 30> LiteVolumeSegments;
typedef StaticArray<VolumeSegment, 30> VolumeSegments;
typedef StaticArray<Isect, 30> Intersections;
#endif

// Segment of a ray in one medium
struct VolumeSegment
{
	float mDistMin;         // Distance of the segment begin from the ray origin.
	float mDistMax;         // Distance of the segment end from the ray origin.
	float mRaySamplePdf;    // If scattering occurred in this medium: PDF of having samples in this segment; otherwise: PDF of passing through the entire medium.
	float mRaySampleRevPdf; // Similar to mRaySamplePdf but in a reverse direction.
	Rgb   mAttenuation;     // Attenuation caused by this segment (not divide by PDF).
	Rgb   mEmission;        // Emission coming from this segment (neither attenuated, nor divided by PDF).
	int   mMediumID;		// ID of the medium in this segment.

	// Accumulates attenuation from all the given segments (just multiplies them together).
	static Rgb AccumulateAttenuationWithoutPdf(const VolumeSegments &aSegments)
	{
		Rgb attenuation(1.0f);
		for (VolumeSegments::const_iterator i = aSegments.begin(); i != aSegments.end(); ++i)
		{
			attenuation *= i->mAttenuation;
		}
		return attenuation;
	}

	// Accumulates attenuated emission from all the given segments without dividing by PDF
	// (= attenuation1 * emission1 + attenuation1 * attenuation2 * emission2 + attenuation1 * attenuation2 * attenuation3 * emission3 + ... ).
	static Rgb AccumulateAttenuatedEmissionWithoutPdf(const VolumeSegments &aSegments)
	{
		Rgb attenuation(1.0f);
		Rgb emission(0.0f);
		for (VolumeSegments::const_iterator i = aSegments.begin(); i != aSegments.end(); ++i)
		{
			attenuation *= i->mAttenuation;
			emission += i->mEmission *attenuation;				
		}
		return emission;
	}

	// Accumulates attenuated emission from all the given segments with dividing by PDF
	// (= attenuation1/pdf1 * emission1 + attenuation1/pdf1 * attenuation2/pdf2 * emission2 + attenuation1/pdf1 * attenuation2/pdf2 * attenuation3/pdf3 * emission3 + ... ).
	static Rgb AccumulateAttenuatedEmissionWithPdf(const VolumeSegments &aSegments)
	{
		Rgb attenuation(1.0f);
		Rgb emission(0.0f);
		float pdf(1.0f);
		for (VolumeSegments::const_iterator i = aSegments.begin(); i != aSegments.end(); ++i)
		{
			attenuation *= i->mAttenuation;
			pdf *= i->mRaySamplePdf;
			emission += i->mEmission * attenuation / pdf;				
		}
		return emission;
	}

	// Accumulates attenuated emission from all the given segments with dividing by reverse PDF
	// (= attenuation1/revpdf1 * emission1 + attenuation1/revpdf1 * attenuation2/revpdf2 * emission2 + attenuation1/revpdf1 * attenuation2/revpdf2 * attenuation3/revpdf3 * emission3 + ... ).
	static Rgb AccumulateAttenuatedEmissionWithRevPdf(const VolumeSegments &aSegments)
	{
		Rgb attenuation(1.0f);
		Rgb emission(0.0f);
		float revPdf(1.0f);
		for (VolumeSegments::const_iterator i = aSegments.begin(); i != aSegments.end(); ++i)
		{
			attenuation *= i->mAttenuation;
			revPdf *= i->mRaySampleRevPdf;
			emission += i->mEmission * attenuation / revPdf;				
		}
		return emission;
	}

	// Accumulates PDFs from all the given segments (just multiplies them together).
	static float AccumulatePdf(const VolumeSegments &aSegments)
	{
		float pdf(1.0f);
		for (VolumeSegments::const_iterator i = aSegments.begin(); i != aSegments.end(); ++i)
		{
			pdf *= i->mRaySamplePdf;
		}
		return pdf;
	}

	// Accumulates reverse PDFs from all the given segments (just multiplies them together).
	static float AccumulateRevPdf(const VolumeSegments &aSegments)
	{
		float revPdf(1.0f);
		for (VolumeSegments::const_iterator i = aSegments.begin(); i != aSegments.end(); ++i)
		{
			revPdf *= i->mRaySampleRevPdf;
		}
		return revPdf;
	}
};

// When we don't need PDFs etc.
struct VolumeSegmentLite
{
	float mDistMin;
	float mDistMax;
	int   mMediumID;
};

#endif //__RAY_HXX__