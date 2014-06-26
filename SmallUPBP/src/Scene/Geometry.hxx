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

#ifndef __GEOMETRY_HXX__
#define __GEOMETRY_HXX__

#include <vector>
#include <cmath>
#include <list>
#include <algorithm>
#include <omp.h>

#include "include\embree.h"
#include "common\ray.h"
#include "..\Misc\Utils2.hxx"
#include "..\Path\Ray.hxx"
#include "Materials.hxx"


//////////////////////////////////////////////////////////////////////////
// Geometry

enum GeometryPrimitiveType
{
	GEOM_TRIANGLE = 0,
	GEOM_OTHER = 1
};

class AbstractGeometry : public embree::Intersector1
{
public:

	AbstractGeometry(GeometryPrimitiveType aType) :mType(aType), embree::Intersector1(embreeIntersect,embreeOccluded){}

	virtual ~AbstractGeometry(){}

    // Finds the closest intersection
    virtual bool Intersect (const Ray& aRay, Isect &oIntersection) const = 0;

	// Finds all intersections with the given ray, returns them in sorted list, default calls Intersect
    virtual void IntersectAll(const Ray& aRay, const float aMaxDist, Intersections & oIntersections) const
    {
		Isect isect(aMaxDist);
		if ( Intersect(aRay, isect) )
			oIntersections.push_back(isect);
    }
	
    // Grows given bounding box by this object
    virtual void GrowBBox(Pos &aoBBoxMin, Pos &aoBBoxMax) = 0;

	// Computes additional info about given intersection
	virtual void computeIntersectionInfo(Isect & oIntersection) const
	{
		/// Should never be called for certain geometry classes
		UPBP_ASSERT(false);
	}

	// Returns geometry type
	GeometryPrimitiveType getType() const { return mType; }

	// Converts embree ray to our ray
	static  __forceinline Ray rayConvert(const embree::Ray & aRay)
	{
		Ray r;
		r.direction = Dir(aRay.dir.x, aRay.dir.y, aRay.dir.z);
		r.origin = Pos(aRay.org.x, aRay.org.y, aRay.org.z);
		return r;
	}

	// Converts embree ray to our ray
	static  __forceinline embree::Ray rayConvert(const Ray & aRay, float aMaxDistance)
	{
		embree::Ray r;
		r.dir = embree::Vec3fa(aRay.direction.x(), aRay.direction.y(), aRay.direction.z());
		r.org = embree::Vec3fa(aRay.origin.x(), aRay.origin.y(), aRay.origin.z());
		r.tnear = 0.0f;
		r.tfar = aMaxDistance;
		r.id0 = r.id1 = -1;
		return r;
	}

	// Embree accelerated intersection test
	static void embreeIntersect(const embree::Intersector1* This, embree::Ray& ray)
	{
		const AbstractGeometry * geom = (const AbstractGeometry *)(This);
		Isect & tmp = *sIntersections[omp_get_thread_num()];
		if (geom->Intersect(rayConvert(ray), tmp))
		{
			ray.tfar = tmp.mDist;
			ray.id0 = ray.id1 = geom->mId;
		}
	}

	// Embree accelerated occlusion test
	static bool embreeOccluded(const embree::Intersector1* This, embree::Ray& ray)
	{
		Isect tmp;
		tmp.mDist = ray.tfar;
		const AbstractGeometry * geom = (const AbstractGeometry *)(This);
		bool occ = geom->Intersect(rayConvert(ray), tmp);
		occ &= tmp.mDist >= ray.tnear && tmp.mDist <= ray.tfar;
		return occ;
	}

	// Sets id for embree intersection test
	void setId(int id)
	{
		mId = id;
	}

	/// Returns id for embree intersection test
	int getId() const
	{
		return mId;
	}

	/// Allocates static resources for reporting Intersections
	static void allocStaticResources()
	{
		if (sIntersections == nullptr)
		{
			sIntersections = new Isect *[omp_get_max_threads()];
		}
	}

	/// Deallocates static resources for reporting Intersections
	static void deallocStaticResources()
	{
		if (sIntersections != nullptr)
		{
			delete[] sIntersections;
			sIntersections = nullptr;
		}
	}

	/// Sets whether we will compute shading normal
	static void setUseShadingNormal(bool use)
	{
		sUseShadingNormal = use;
	}

	/// Returns whether we will compute shading normal
	static bool useShadingNormal()
	{
		return sUseShadingNormal;
	}
private:
	GeometryPrimitiveType mType; /// Geometry type
	int mId; /// Id for reporting embree intersection Intersections
	static bool sUseShadingNormal; ///If false, it sets shading normal equal to geometry normal
protected:
	/// Static variables - one element of array for each thread
	static Isect ** sIntersections; /// For reporting Intersections
};

class Triangle : public AbstractGeometry
{
public:

	Triangle() :AbstractGeometry(GEOM_TRIANGLE){}

    Triangle(
        const Pos &p0,
        const Pos &p1,
        const Pos &p2,
        int       aMatID,
		int       aMedID = -1,
		int       aLightID = -1)
		:AbstractGeometry(GEOM_TRIANGLE)
    {
        p[0] = p0;
        p[1] = p1;
        p[2] = p2;
        matID = aMatID;
		medID = aMedID;
		lightID = aLightID;
		mNormal = (cross(p[1] - p[0], p[2] - p[0])).getNormalized();
		n[0] = mNormal;
		n[1] = mNormal;
		n[2] = mNormal;
    }

	virtual bool Intersect(
		const Ray &aRay,
		Isect     &oIntersection) const
	{
		/* Find vectors for two edges sharing */
		Dir edge1 = p[1] - p[0], edge2 = p[2] - p[0];

		/* Begin calculating determinant - also used to calculate U parameter */
		Dir pvec = cross(aRay.direction, edge2);

		float det = dot(edge1, pvec);
		if (det == 0)
			return false;
		float inv_det = 1.0f / det;

		/* Calculate distance from p[0] to ray origin */
		Dir tvec = aRay.origin - p[0];

		/* Calculate U parameter and test bounds */
		float u = dot(tvec, pvec) * inv_det;
		if (u < 0.0 || u > 1.0)
			return false;

		/* Prepare to test V parameter */
		Dir qvec = cross(tvec, edge1);

		/* Calculate V parameter and test bounds */
		float v = dot(aRay.direction, qvec) * inv_det;

		/* Inverted comparison (to catch NaNs) */
		if (v >= 0.0 && u + v <= 1.0) {
			/* ray intersects triangle -> compute t */
			float distance = dot(edge2, qvec) * inv_det;

			if ((distance > 0) & (distance < oIntersection.mDist))
			{
				oIntersection.mDist = distance;
				oIntersection.mMatID = matID;
				oIntersection.mMedID = medID;
				oIntersection.mLightID = lightID;
				oIntersection.mNormal = mNormal;
				oIntersection.mEnter = dot(mNormal, aRay.direction) < 0;
				oIntersection.mUV = Vec2f(u, v);
				oIntersection.mElementID = getId();
				return true;
			}
		}
		return false;
	}
	
    virtual void GrowBBox(
        Pos &aoBBoxMin,
        Pos &aoBBoxMax)
    {
        for(int i=0; i<3; i++)
        {
            for(int j=0; j<3; j++)
            {
                aoBBoxMin[j] = std::min(aoBBoxMin[j], p[i][j]);
                aoBBoxMax[j] = std::max(aoBBoxMax[j], p[i][j]);
            }
        }
    }

	// Computes additional info about given intersection
	virtual void computeIntersectionInfo(Isect & oIntersection) const
	{
		if (useShadingNormal())
		{
			oIntersection.mShadingNormal = n[1] * oIntersection.mUV.x + n[2] * oIntersection.mUV.y + n[0] * (1 - oIntersection.mUV.x - oIntersection.mUV.y);
			oIntersection.mShadingNormal = oIntersection.mShadingNormal.getNormalized();
		}
		else
		{
			oIntersection.mShadingNormal = mNormal;
		}
	}

public:

    Pos p[3];
	Dir n[3];
	Vec2f t[3];
    int matID;
	int medID;
	int lightID;
    Dir mNormal;
};

class Sphere : public AbstractGeometry
{
public:

	Sphere() :AbstractGeometry(GEOM_OTHER){}
    
    Sphere(
        const Pos   &aCenter,
		float       aRadius,
        int         aMatID,
		int         aMedID = -1,
		int         aLightID = -1)
	:AbstractGeometry(GEOM_OTHER)
    {
        center  = aCenter;
        radius  = aRadius;
        matID   = aMatID;
		medID   = aMedID;
		lightID = aLightID;
    }

	virtual bool Intersect(
		const Ray &aRay,
		Isect     &oIntersection) const
	{
		return Intersect(aRay, oIntersection, NULL);
	}

    // Taken from:
    // http://wiki.cgsociety.origin/index.php/Ray_Sphere_Intersection

    bool Intersect(
        const Ray &aRay,
        Isect     &oIntersection,
		Isect     *oSecondIntersection) const
    {
        // we transform ray origin into object space (center == origin)
        const Dir transformedOrigin = aRay.origin - center;

        const float A = dot(aRay.direction, aRay.direction);
		const float B = 2 * dot(aRay.direction, transformedOrigin);
		const float C = dot(transformedOrigin, transformedOrigin) - (radius * radius);

        // Must use doubles, because when B ~ sqrt(B*B - 4*A*C)
        // the resulting t is imprecise enough to get around ray epsilons
        const double disc = B*B - 4*A*C;

        if(disc <= 0)
            return false;

        const double discSqrt = std::sqrt(disc);
        const double q = (B < 0) ? ((-B - discSqrt) / 2.f) : ((-B + discSqrt) / 2.f);

        double t0 = q / A;
        double t1 = C / q;

        if(t0 > t1) std::swap(t0, t1);

        float resT;
		float resT2;

		if (t0 > 0 && t0 < oIntersection.mDist)
		{
			resT = float(t0);
			resT2 = float(t1);
		}
		else if (t1 > 0 && t1 < oIntersection.mDist)
		{
			resT = float(t1);
			resT2 = float(t0);
		}            
        else
            return false;

        oIntersection.mDist    = resT;
		oIntersection.mMatID   = matID;
		oIntersection.mMedID   = medID;
		oIntersection.mLightID = lightID;
		oIntersection.mNormal  = (transformedOrigin + Dir(resT) * aRay.direction).getNormalized();
		oIntersection.mEnter   = C > 0;
		oIntersection.mElementID = getId();
		oIntersection.mUV = Vec2f(0, 0); // Unsupported

		if (oSecondIntersection)
		{
			oSecondIntersection->mDist = resT2;
			oSecondIntersection->mMatID = matID;
			oSecondIntersection->mMedID = medID;
			oSecondIntersection->mLightID = lightID;
			oSecondIntersection->mNormal = (transformedOrigin + Dir(resT2) * aRay.direction).getNormalized();
			oSecondIntersection->mEnter = false;
			oSecondIntersection->mElementID = getId();
			oSecondIntersection->mUV = Vec2f(0, 0); // Unsupported
		}
		
        return true;
    }

	virtual void IntersectAll(const Ray& aRay, const float aMaxDist, Intersections & oIntersections) const
	{
		Isect isect(aMaxDist);
		Isect isect2(aMaxDist);

		// Try find the first intersection
		if (Intersect(aRay, isect, &isect2))
		{
			oIntersections.push_back(isect);

			// If the first intersection returned is the entry intersection, the second one is the exit
			if (isect.mEnter && isect2.mDist < aMaxDist)
				oIntersections.push_back(isect2);
		}
	}

    virtual void GrowBBox(
        Pos &aoBBoxMin,
        Pos &aoBBoxMax)
    {
        for(int i=0; i<8; i++)
        {
            Pos p = center;
            Dir half(radius);

            for(int j=0; j<3; j++)
                if(i & (1 << j)) half[j] = -half[j];
            
            p += half;

            for(int j=0; j<3; j++)
            {
                aoBBoxMin[j] = std::min(aoBBoxMin[j], p[j]);
                aoBBoxMax[j] = std::max(aoBBoxMax[j], p[j]);
            }
        }
    }

	// Computes additional info about given intersection
	virtual void computeIntersectionInfo(Isect & oIntersection) const
	{
		oIntersection.mShadingNormal = oIntersection.mNormal;
	}

private:
	bool inside(Pos point) const
	{
		Dir dir = point - center;
		return dot(dir, dir) <= radius * radius;
	}

public:

    Pos    center;
	float  radius;
    int    matID;
	int    medID;
	int    lightID;
};

class GeometryList : public AbstractGeometry
{
public:

	GeometryList() : AbstractGeometry(GEOM_OTHER) {}

	virtual ~GeometryList()
	{
		for (int i = 0; i < (int)mGeometry.size(); i++)
			delete mGeometry[i];
	};

	virtual bool Intersect(const Ray& aRay, Isect &oIntersection) const
	{
		bool anyIntersection = false;

		for (int i = 0; i < (int)mGeometry.size(); i++)
		{
			bool hit = mGeometry[i]->Intersect(aRay, oIntersection);

			if (hit)
				anyIntersection = hit;
		}

		return anyIntersection;
	}

	virtual void IntersectAll(const Ray& aRay, const float aMaxDist, Intersections & oIntersections) const
	{
		// Gather all intersections with each geometry in the list
		for (int i = 0; i < (int)mGeometry.size(); i++)
		{
			mGeometry[i]->IntersectAll(aRay, aMaxDist, oIntersections);
		}

		// Sort them
		oIntersections.sort();
	}

	// Not only grows BBox, but also builds structure for faster ray intersection routines
	virtual void GrowBBox(
		Pos &aoBBoxMin,
		Pos &aoBBoxMax)
	{
		for (int i = 0; i < (int)mGeometry.size(); i++)
		{
			mGeometry[i]->GrowBBox(aoBBoxMin, aoBBoxMax);
			mGeometry[i]->setId(i);
		}
	}

public:
	std::vector<AbstractGeometry*> mGeometry; // All geometry in small upbp internal format
};

class AcceleratedGeometryList : public GeometryList
{
public:

	virtual ~AcceleratedGeometryList()
	{
		embree::rtcDeleteIntersector1(mMeshIntersector);
		embree::rtcDeleteGeometry(mMesh);
		embree::rtcDeleteIntersector1(mOtherIntersector);
		embree::rtcDeleteGeometry(mOtherGeometry);
		AbstractGeometry::deallocStaticResources();
	};


	virtual bool Intersect(const Ray& aRay, Isect &oIntersection) const
	{
		sIntersections[omp_get_thread_num()] = &oIntersection;
		embree::Ray ray = AbstractGeometry::rayConvert(aRay, oIntersection.mDist);
		mMeshIntersector->intersect(ray);
		oIntersection.mElementID = -1;
		if (ray.id0 >= 0) // Hit
		{
			const Triangle * tr = (const Triangle *)(mGeometry[ray.id0]);
			// Triangle hit
			oIntersection.mDist = ray.tfar;
			oIntersection.mMatID = tr->matID;
			oIntersection.mMedID = tr->medID;
			oIntersection.mLightID = tr->lightID;
			oIntersection.mNormal = tr->mNormal;
			oIntersection.mElementID = ray.id0;
			oIntersection.mUV = Vec2f(ray.u, ray.v);
			oIntersection.mEnter = dot(tr->mNormal, aRay.direction) < 0;
			ray = AbstractGeometry::rayConvert(aRay, ray.tfar);
		}
		if (!mAnyNonTriangles)
			return oIntersection.mElementID >= 0;
		//mOtherIntersector->intersect(ray);
		for (int i = 0; i < (int)mOtherGeometryList.size(); i++)
		{
			mOtherGeometryList[i]->Intersect(aRay, oIntersection);
		}
		return oIntersection.mElementID >=0;
	}

	// Not only grows BBox, but also builds structure for faster ray intersection routines
	virtual void GrowBBox(
		Pos &aoBBoxMin,
		Pos &aoBBoxMax);

public:

	embree::RTCGeometry* mMesh; // Triangle geometry

	embree::RTCIntersector1* mMeshIntersector; // Intersector for triangle geometry

	embree::RTCGeometry * mOtherGeometry; // Other geometry

	embree::RTCIntersector1* mOtherIntersector; // Intersector for 

	bool mAnyNonTriangles; // Any other geometry

	std::vector<AbstractGeometry*> mOtherGeometryList; // All geometry in small upbp internal format
};

#endif //__GEOMETRY_HXX__
