// ======================================================================== //
// Copyright 2009-2013 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#ifndef __EMBREE_RAY_H__
#define __EMBREE_RAY_H__

#include "../common/default.h"

class AbstractMedium;
class Scene;
namespace SmallUPBP
{
	class Ray;
}

namespace embree
{
	struct AdditionalRayDataForMis
	{
		void*                 mLightVertices;           // type std::vector<UPBPLightVertex>*
		void*                 mPathEnds;                // type std::vector<int>*
		void*                 mCameraVerticesMisData;   // type MisData*		
		const unsigned int    mCameraPathLength;
		const unsigned int    mMinPathLength;
		const unsigned int    mMaxPathLength;
		const unsigned int    mQueryBeamType;
		const unsigned int    mPhotonBeamType;
		const float           mLastPdfWInv;
		const float           mSurfMisWeightFactor;
		const float           mPP3DMisWeightFactor;
		const float           mPB2DMisWeightFactor;
		const float           mBB1DMisWeightFactor;
		void*                 mBB1DPhotonBeams;
		const float           mBB1DMinMFP;
		const float           mLightSubPathCount;
		const float           mMinDistToMed;		
		float                 mRaySamplePdf;
		float                 mRaySampleRevPdf;
		unsigned int          mRaySamplingFlags;
		void				  *mDebugImages;			// Pointer to DebugImages class

		AdditionalRayDataForMis(
			void*                 aLightVertices,
			void*                 aPathEnds,
			void*                 aCameraVerticesMisData,			
			const unsigned int    aCameraPathLength,
			const unsigned int    aMinPathLength,
			const unsigned int    aMaxPathLength,
			const unsigned int    aQueryBeamType,
			const unsigned int    aPhotonBeamType,
			const float           aLastPdfWInv,		
			const float           aSurfMisWeightFactor,
			const float           aPP3DMisWeightFactor,
			const float           aPB2DMisWeightFactor,
			const float           aBB1DMisWeightFactor,
			void*                 aBB1DPhotonBeams,
			const float           aBB1DMinMFP,
			const float           aLightSubPathCount,
			const float           aMinDistToMed,
			const float           aRaySamplePdf = 0,
			const float           aRaySampleRevPdf = 0,
			const unsigned int    aRaySamplingFlags = 0,
			void				  *aDebugImages = 0) :
			mLightVertices(aLightVertices),
			mPathEnds(aPathEnds),
			mCameraVerticesMisData(aCameraVerticesMisData),				
			mCameraPathLength(aCameraPathLength),
			mMinPathLength(aMinPathLength),
			mMaxPathLength(aMaxPathLength),
			mQueryBeamType(aQueryBeamType),
			mPhotonBeamType(aPhotonBeamType),
			mLastPdfWInv(aLastPdfWInv),
			mSurfMisWeightFactor(aSurfMisWeightFactor),
			mPP3DMisWeightFactor(aPP3DMisWeightFactor),
			mPB2DMisWeightFactor(aPB2DMisWeightFactor),
			mBB1DMisWeightFactor(aBB1DMisWeightFactor),
			mBB1DPhotonBeams(aBB1DPhotonBeams),
			mBB1DMinMFP(aBB1DMinMFP),
			mLightSubPathCount(aLightSubPathCount),
			mMinDistToMed(aMinDistToMed),
			mRaySamplePdf(aRaySamplePdf),
			mRaySampleRevPdf(aRaySampleRevPdf),
			mRaySamplingFlags(aRaySamplingFlags),
			mDebugImages(aDebugImages)
		{}
	};
	
	/*! Ray structure. Contains all information about a ray including
   *  precomputed reciprocal direction. */
  struct Ray
  {
    /*! Default construction does nothing. */
    __forceinline Ray() {}

    /*! Constructs a ray from origin, direction, and ray segment. Near
     *  has to be smaller than far. */
    __forceinline Ray(const Vector3f& org, const Vector3f& dir, float tnear = zero, float tfar = inf, float time = zero, int mask = -1)
      : org(org), dir(dir), tnear(tnear), tfar(tfar), id0(-1), id1(-1), mask(mask), time(time) {}

	__forceinline void setAdditionalData(const AbstractMedium* aMedium, void* aAccumResult, unsigned int aFlags, const SmallUPBP::Ray * aOrigRay, const AdditionalRayDataForMis * aAdditionalRayDataForMis = NULL)
	{
		medium = aMedium;
		accumResult = aAccumResult;
		flags = aFlags;
		origRay = aOrigRay;
		additionalRayDataForMis = aAdditionalRayDataForMis;
	}

    /*! Tests if we hit something. */
    __forceinline operator bool() const { return id0 != -1; }

  public:
    Vec3fa org;        //!< Ray origin
    Vec3fa dir;        //!< Ray direction
    float tnear;       //!< Start of ray segment
    float tfar;        //!< End of ray segment
    float u;           //!< Barycentric u coordinate of hit
    float v;           //!< Barycentric v coordinate of hit
    int id0;           //!< 1st primitive ID
    int id1;           //!< 2nd primitive ID
    Vec3fa Ng;         //!< Not normalized geometry normal
    int mask;          //!< used to mask out objects during traversal
    float time;        //!< Time of this ray for motion blur
	
	size_t                         invocation;
	const AbstractMedium*          medium;                  //!< Medium that this ray passes through (used for BRE and photon beams)
	void*                          accumResult;             //!< Result radiance calculation alongthe ray (in BRE and photon beams), type Rgb*
	unsigned int                   flags;                   //!< Additional flags
	const SmallUPBP::Ray*          origRay;                 //!< Original ray
	const AdditionalRayDataForMis* additionalRayDataForMis; //!< Additional data needed for MIS weights computation
  };

  /*! Outputs ray to stream. */
  inline std::ostream& operator<<(std::ostream& cout, const Ray& ray) {
    return cout << "{ " << 
      "org = " << ray.org << ", dir = " << ray.dir << ", near = " << ray.tnear << ", far = " << ray.tfar << ", time = " << ray.time << ", " <<
      "id0 = " << ray.id0 << ", id1 = " << ray.id1 <<  ", " << "u = " << ray.u <<  ", v = " << ray.v << ", Ng = " << ray.Ng << " }";
  }
}

#endif
