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

#ifndef __PHOTONBEAM_HXX__
#define __PHOTONBEAM_HXX__

#include <vector>

#include "common\ray.h"
#include "..\Misc\Debugimages.hxx"
#include "..\Structs\BoundingBox.hxx"
#include "..\Path\PathWeight.hxx"

/**
 * @brief	Photon beam.
 */
struct PhotonBeam
{
	// Filled by MC algorithm:

	Rgb		mThroughputAtOrigin;	//!< Path throughput (including emission and division by path PDF) at beam origin.
	Ray		mRay;					//!< Origin and direction of the photon beam.
	float   mLength;				//!< Beam length.
	uint	mFlags;					//!< Various flags.
	float   mRaySamplePdf;          //!< PDF of sampling through previous media on the generating ray.
	float   mRaySampleRevPdf;       //!< Reverse PDF of sampling through previous media on the generating ray.
	uint    mRaySamplingFlags;      //!< Flags for sampling inside the beam.
	float   mLastPdfWInv;           //!< Direction sampling PDF of the generating ray.
	bool    mIsFirstSegment;        //!< Whether this beam corresponds to the first volume segment on the generating ray.
	const AbstractMedium * mMedium;	//!< Medium in which beam resides.
	UPBPLightVertex* mLightVertex;  //!< Light vertex at the origin of the generating ray.
	
	// Filled by PhotonBeamsEvaluator:

	float  mStartRadius;			//!< Beam's start radius.
	float  mEndRadius;				//!< Beam's end radius.
	float  mMaxRadiusSqr;			//!< Beam's max radius squared.
	float  mRadiusChange;			//!< Beams's radius change = (mEndRadius - mStartRadius) / mLength.
	mutable size_t  mInvocation;    //!< Beam invoke parameter for KD-tree tracing.
	mutable BoundingBox3 mAABB;	    //!< Beam's bounding box (for faster clipping operations).
	mutable Dir mMargins;			//!< Beam's margins - computed during first AABB call.

	/**
	 * @brief	Compute the AABB of a beam (only used during tree construction).
	 *
	 * @return	The AABB of a beam.
	 */
	inline BoundingBox3 getAABB() const {
		if (mAABB.isEmpty())
		{
			const Dir dirSqr = mRay.direction*mRay.direction; // component-wise multiply
			const Dir marginsSqr = Dir(
				dirSqr.y() + dirSqr.z(),
				dirSqr.x() + dirSqr.z(),
				dirSqr.x() + dirSqr.y()
				);
			mMargins = marginsSqr.sqrt();
			mAABB = getSegmentAABB(0, 1);
		}
		return mAABB;
	}

	/**
	 * @brief	Compute the clipped AABB of a beam (only used during tree construction) - requires
	 * 			already computed mAABB !!!
	 *
	 * @param	splitAxis	Index of the splitting axis.
	 * @param	split	 	The split.
	 *
	 * @return	The clipped AABB.
	 */
	inline BoundingBox3 getClippedAABBLeft(int splitAxis, float split) const {
		if (unlikely(mRay.direction[splitAxis] == 0.0f)) // Parallel
		{
			UPBP_ASSERT(!mAABB.isEmpty());
			Pos maxPt = mAABB.point2;
			maxPt[splitAxis] = split;
			return BoundingBox3(mAABB.point1, maxPt);
		}
		else
		{
			const float splitT = (split - mRay.origin[splitAxis]) / mRay.direction[splitAxis];
			const Pos & beamStart = mRay.origin;
			const Pos beamEnd = mRay.target(splitT);
			const float & radiusStart = mStartRadius;
			const float radiusEnd = mStartRadius + splitT * mRadiusChange;
			const Dir startMargins = radiusStart * mMargins;
			const Dir endMargins = radiusEnd * mMargins;
			const Pos minPt = Pos::min(beamStart - startMargins, beamEnd - endMargins);
			const Pos maxPt = Pos::max(beamStart + startMargins, beamEnd + endMargins);
			return BoundingBox3(minPt, maxPt);
		}
	}

	/**
	 * @brief	Compute the clipped AABB of a beam (only used during tree construction) - requires
	 * 			already computed mAABB !!!
	 *
	 * @param	splitAxis	Index of the splitting axis.
	 * @param	split	 	The split.
	 *
	 * @return	The clipped AABB.
	 */
	inline BoundingBox3 getClippedAABBRight(int splitAxis, float split) const {
		if (unlikely(mRay.direction[splitAxis] == 0.0f)) // Parallel
		{
			UPBP_ASSERT(!mAABB.isEmpty());
			Pos minPt = mAABB.point1;
			minPt[splitAxis] = split;
			return BoundingBox3(minPt, mAABB.point2);
		}
		else
		{
			const float splitT = (split - mRay.origin[splitAxis]) / mRay.direction[splitAxis];
			const Pos beamStart = mRay.target(splitT);
			const Pos beamEnd = mRay.target(mLength);
			const float radiusStart = mStartRadius + splitT * mRadiusChange;
			const float & radiusEnd = mEndRadius;
			const Dir startMargins = radiusStart * mMargins;
			const Dir endMargins = radiusEnd * mMargins;
			const Pos minPt = Pos::min(beamStart - startMargins, beamEnd - endMargins);
			const Pos maxPt = Pos::max(beamStart + startMargins, beamEnd + endMargins);
			return BoundingBox3(minPt, maxPt);
		}
	}

	/**
	 * @brief	Compute the clipped AABB of a beam (only used during tree construction).
	 *
	 * @param	other	The other AABB.
	 *
	 * @return	The clipped AABB.
	 */
	inline BoundingBox3 getClippedAABB(const BoundingBox3 & other) const {
		float mint, maxt;
		if (likely(other.intersect(mRay, 1.0f / mRay.direction, mint, maxt)))
		{
			if (unlikely(maxt < 0 || mint > mLength))
				return BoundingBox3();
			maxt = std::min(maxt, mLength);
			mint = std::max(mint, 0.0f);
			const Pos beamStart = mRay.target(mint);
			const Pos beamEnd = mRay.target(maxt);
			const float radiusStart = mStartRadius + mint * mRadiusChange;
			const float radiusEnd = mStartRadius + maxt * mRadiusChange;
			const Dir startMargins = radiusStart * mMargins;
			const Dir endMargins = radiusEnd * mMargins;
			const Pos minPt = Pos::min(beamStart - startMargins, beamEnd - endMargins);
			const Pos maxPt = Pos::max(beamStart + startMargins, beamEnd + endMargins);
			return BoundingBox3(minPt, maxPt).getIntersection(other);
		}
		return BoundingBox3();
	}

	/**
	 * @brief	Compute the AABB of a segment (only used during tree construction).
	 *
	 * @param	splitMin	The split minimum.
	 * @param	splitMax	The split maximum.
	 *
	 * @return	The segment AABB.
	 */
	inline BoundingBox3 getSegmentAABB(float splitMin, float splitMax) const {
		
		splitMin *= mLength;
		splitMax *= mLength;
		const Pos beamStart = mRay.target(splitMin);
		const Pos beamEnd = mRay.target(splitMax);
		const float radiusStart = mStartRadius + splitMin * mRadiusChange;
		const float radiusEnd = mStartRadius + splitMax * mRadiusChange;
		const Dir startMargins = radiusStart * mMargins;
		const Dir endMargins = radiusEnd * mMargins;
		const Pos minPt = Pos::min(beamStart - startMargins, beamEnd - endMargins);
		const Pos maxPt = Pos::max(beamStart + startMargins, beamEnd + endMargins);
		return BoundingBox3(minPt, maxPt);
		// NOT-TIGHT:
		/*splitMin *= mLength;
		splitMax *= mLength;
		const Pos beamStart = mRay.target(splitMin);
		const Pos beamEnd = mRay.target(splitMax);
		const Pos minPt = Pos::min(beamStart, beamEnd);
		const Pos maxPt = Pos::max(beamStart, beamEnd);
		const Dir radius = Dir(std::max(mStartRadius, mEndRadius));
		return BoundingBox3(minPt-radius, maxPt+radius);*/
	}

	/**
	 * @brief	Intersection of ray with beam.
	 * 			
	 * 			The query ray is specified by (O1,d1) [minT1, maxT1). The photon beam segment is
	 * 			specified by (O2,d2) [minT2, maxT2). Only intersection within distance
	 * 			sqrt(maxDistSqr) are reported.
	 *
	 * @param	O1				 	Query ray origin.
	 * @param	d1				 	Query ray direction.
	 * @param	minT1			 	Query ray minimum t.
	 * @param	maxT1			 	Query ray maximum t.
	 * @param	O2				 	Photon beam origin.
	 * @param	d2				 	Photon beam direction.
	 * @param	minT2			 	Photon beam minimum t.
	 * @param	maxT2			 	Photon beam maximum t.
	 * @param	maxDistSqr		 	The maximum distance squared.
	 * @param [in,out]	oDistance	The distance between the two lines.
	 * @param [in,out]	oSinTheta	The sine of the angle between the two lines.
	 * @param [in,out]	oT1		 	The ray parameter of the closes point along the query ray.
	 * @param [in,out]	oT2		 	The ray parameter of the closes point along the photon beam segment.
	 *
	 * @return	true is an intersection is found, false otherwise. If an intersection is found, the
	 * 			following output parameters are set: 
	 * 			oDistance ... distance between the two lines
	 * 			oSinTheta ... sine of the angle between the two lines 
	 * 			oT1 ......... ray parameter of the closes point along the query ray 
	 * 			oT2 ......... ray parameter of the closes point along the photon beam segment.
	 */
	static __forceinline bool testIntersectionBeamBeam(
		const Pos& O1,
		const Dir& d1,
		const float minT1,
		const float maxT1,
		const Pos& O2,
		const Dir& d2,
		const float minT2,
		const float maxT2,
		const float maxDistSqr,
		float& oDistance,
		float& oSinTheta,
		float& oT1,
		float& oT2)
	{
		const Dir  d1d2c = cross(d1, d2);
		const float sinThetaSqr = dot(d1d2c, d1d2c); // Square of the sine between the two lines (||cross(d1, d2)|| = sinTheta).

		// Slower code to test if the lines are too far apart.
		// oDistance = absDot((O2 - O1), d1d2c) / d1d2c.size();
		// if(oDistance*oDistance >= maxDistSqr) return false; 

		const float ad = dot((O2 - O1), d1d2c);

		// Lines too far apart.
		if (ad*ad >= maxDistSqr*sinThetaSqr)
			return false;

		// Cosine between the two lines.
		const float d1d2 = dot(d1, d2);
		const float d1d2Sqr = d1d2*d1d2;
		const float d1d2SqrMinus1 = d1d2Sqr - 1.0f;

		// Parallel lines?
		if (unlikely(d1d2SqrMinus1 < 1e-5f && d1d2SqrMinus1 > -1e-5f))
			return false;

		const float d1O1 = dot(d1, O1);
		const float d1O2 = dot(d1, O2);

		oT1 = (d1O1 - d1O2 - d1d2 * (dot(d2, O1) - dot(d2, O2))) / d1d2SqrMinus1;

		// Out of range on ray 1.
		if (unlikely(oT1 <= minT1 || oT1 >= maxT1))
			return false;

		oT2 = (oT1 + d1O1 - d1O2) / d1d2;
		// Out of range on ray 2.
		if (oT2 <= minT2 || oT2 >= maxT2 || _isnanf(oT2))
			return false;

		const float sinTheta = std::sqrt(sinThetaSqr);

		oDistance = fabsf(ad) / sinTheta;

		oSinTheta = sinTheta;

		return true; // Found an intersection.
	}

	/**
	 * @brief	Accumulates photon beam contribution to ray with given [mint,maxt] and flags,
	 * 			accumulation is stored to \c accumResult.
	 * 			
	 * 			Version used by pure BB1D algorithm from \c VolLightTracer.hxx.
	 *
	 * @param	ray						The query ray.
	 * @param	mint					Original minimum value of the ray t parameter.
	 * @param	maxt					Original maximum value of the ray t parameter.
	 * @param	isectmint				Minimum value of the ray t parameter inside the tested cell.
	 * @param	isectmaxt				Maximum value of the ray t parameter inside the tested cell.
	 * @param	beamSelectionPdf		PDF of testing a beam in the tested cell.
	 * @param [in,out]	accumResult 	Accumulated result.
	 * @param	rayFlags				Ray flags (beam type and estimator techniques).
	 * @param	medium					Medium the current ray segment is in.
	 * @param	additionalDataForMis	(Optional) the additional data for MIS weights computation.
	 */
	__forceinline void accumulate(const Ray &ray, const float mint, const float maxt, const float isectmint, const float isectmaxt, const float beamSelectionPdf, Rgb & accumResult, uint rayFlags, const AbstractMedium * medium, const embree::AdditionalRayDataForMis* additionalDataForMis = NULL) const {
		if (additionalDataForMis)
		{
			accumulate2(ray, mint, maxt, isectmint, isectmaxt, beamSelectionPdf, accumResult, rayFlags, medium, additionalDataForMis);
			return;
		}

		float beamBeamDistance, sinTheta, queryIsectDist, beamIsectDist;

		if (mMedium == medium && testIntersectionBeamBeam(ray.origin, ray.direction, isectmint, isectmaxt, mRay.origin,
			mRay.direction, 0, mLength, mMaxRadiusSqr, beamBeamDistance, sinTheta, queryIsectDist, beamIsectDist))
		{
			// Compute radius at intersection.
			float radius = mStartRadius + beamIsectDist * mRadiusChange;
			if (radius < beamBeamDistance)
				return;

			// Compute attenuation.
			Rgb attenuation, scattering;
			if (medium->IsHomogeneous())
			{
				const HomogeneousMedium * hmedium = ((const HomogeneousMedium *)medium);
				scattering = hmedium->GetScatteringCoef();
				attenuation = hmedium->EvalAttenuation(queryIsectDist - mint);
				const Rgb beamAtt = hmedium->EvalAttenuation(beamIsectDist);
				if (rayFlags & SHORT_BEAM)
					attenuation /= attenuation[hmedium->mMinPositiveAttenuationCoefCoordIndex()];
				attenuation *= beamAtt;
				if (mFlags & SHORT_BEAM)
					attenuation /= beamAtt[hmedium->mMinPositiveAttenuationCoefCoordIndex()];
			}
			else
			{
				scattering = medium->GetScatteringCoef(ray.target(queryIsectDist));
				attenuation = medium->EvalAttenuation(ray, mint, queryIsectDist);
				attenuation *= medium->EvalAttenuation(mRay, 0, beamIsectDist);
				if (rayFlags & SHORT_BEAM)
					attenuation /= medium->RaySamplePdf(ray, mint, queryIsectDist);
				if (mFlags & SHORT_BEAM)
					attenuation /= medium->RaySamplePdf(mRay, 0, beamIsectDist);
			}

			// Accumulate result.
			accumResult +=
				(1.0f / beamSelectionPdf) * 
				mThroughputAtOrigin *
				scattering *
				attenuation *
				PhaseFunction::Evaluate(ray.direction, mRay.direction, medium->MeanCosine()) *
				(1 - beamBeamDistance * beamBeamDistance / (radius * radius)) * 3 /
				(4 * radius*sinTheta);
		}
	}

	/**
	 * @brief	Accumulates photon beam contribution to ray with given [mint,maxt] and flags,
	 * 			accumulation is stored to \c accumResult.
	 * 			
	 * 			Version used by combined BB1D algorithms from \c UPBP.hxx.
	 *
	 * @param	ray						The query ray.
	 * @param	mint					Original minimum value of the ray t parameter.
	 * @param	maxt					Original maximum value of the ray t parameter.
	 * @param	isectmint				Minimum value of the ray t parameter inside the tested cell.
	 * @param	isectmaxt				Maximum value of the ray t parameter inside the tested cell.
	 * @param	beamSelectionPdf		PDF of testing a beam in the tested cell.
	 * @param [in,out]	accumResult 	Accumulated result.
	 * @param	rayFlags				Ray flags (beam type and estimator techniques).
	 * @param	medium					Medium the current ray segment is in.
	 * @param	additionalDataForMis	(Optional) the additional data for MIS weights computation.
	 */
	__forceinline void accumulate2(const Ray &ray, const float mint, const float maxt, const float isectmint, const float isectmaxt, const float beamSelectionPdf, Rgb & accumResult, uint rayFlags, const AbstractMedium * medium, const embree::AdditionalRayDataForMis* additionalDataForMis = NULL) const 
	{				
		float beamBeamDistance, sinTheta, queryIsectDist, beamIsectDist;

		if (mMedium == medium && testIntersectionBeamBeam(ray.origin, ray.direction, isectmint, isectmaxt, mRay.origin,
			mRay.direction, 0, mLength, mMaxRadiusSqr, beamBeamDistance, sinTheta, queryIsectDist, beamIsectDist))
		{
			UPBP_ASSERT(mLightVertex);
			UPBP_ASSERT(additionalDataForMis);
			UPBP_ASSERT(queryIsectDist);
			UPBP_ASSERT(beamIsectDist);

			// Heterogeneous medium is not supported.
			UPBP_ASSERT(medium->IsHomogeneous());

			// Reject if full path length below/above min/max path length.
			if ((mLightVertex->mPathLength + 1 + additionalDataForMis->mCameraPathLength > additionalDataForMis->mMaxPathLength) ||
				(mLightVertex->mPathLength + 1 + additionalDataForMis->mCameraPathLength < additionalDataForMis->mMinPathLength))
			return;

			// Ignore contribution of primary rays from medium too close to camera.
			if (additionalDataForMis->mCameraPathLength == 1 && queryIsectDist < additionalDataForMis->mMinDistToMed)
				return;

			// Compute radius at intersection.
			float radius = mStartRadius + beamIsectDist * mRadiusChange;
			if (radius < beamBeamDistance)
				return;

			// Compute attenuation in current segment and overall PDFs.
			Rgb attenuation, scattering;
			float raySamplePdfQuery = 1.0f;
			float raySamplePdfBeam = 1.0f;
			float raySampleRevPdfQuery = 1.0f;
			float raySampleRevPdfBeam = 1.0f;
			float raySamplePdfsRatioQuery = 1.0f;
			float raySamplePdfsRatioBeam = 1.0f;
			if (medium->IsHomogeneous())
			{
				const HomogeneousMedium * hmedium = ((const HomogeneousMedium *)medium);
				scattering = hmedium->GetScatteringCoef();							
				// Query
				attenuation = hmedium->EvalAttenuation(queryIsectDist - mint);	
				const float pfdQuery = attenuation[hmedium->mMinPositiveAttenuationCoefCoordIndex()];
				if (rayFlags & SHORT_BEAM)
					attenuation /= pfdQuery;				
				raySamplePdfQuery = hmedium->mMinPositiveAttenuationCoefCoord() * pfdQuery;
				raySampleRevPdfQuery = (additionalDataForMis->mRaySamplingFlags & AbstractMedium::kOriginInMedium) ? raySamplePdfQuery : pfdQuery;
				raySamplePdfsRatioQuery = 1.0f / hmedium->mMinPositiveAttenuationCoefCoord();
				// Beam
				const Rgb beamAtt = hmedium->EvalAttenuation(beamIsectDist);
				attenuation *= beamAtt;
				const float pfdBeam = beamAtt[hmedium->mMinPositiveAttenuationCoefCoordIndex()];
				if (mFlags & SHORT_BEAM)
					attenuation /= pfdBeam;
				raySamplePdfBeam = hmedium->mMinPositiveAttenuationCoefCoord() * pfdBeam;
				raySampleRevPdfBeam = (mRaySamplingFlags & AbstractMedium::kOriginInMedium) ? raySamplePdfBeam : pfdBeam;
				raySamplePdfsRatioBeam = 1.0f / hmedium->mMinPositiveAttenuationCoefCoord();
			}
			else
			{
				scattering = medium->GetScatteringCoef(ray.target(queryIsectDist));
				// Query
				attenuation = medium->EvalAttenuation(ray, mint, queryIsectDist);				
				if (rayFlags & SHORT_BEAM)
					attenuation /= medium->RaySamplePdf(ray, mint, queryIsectDist);
				raySamplePdfQuery = medium->RaySamplePdf(ray, mint, queryIsectDist, additionalDataForMis->mRaySamplingFlags, &raySampleRevPdfQuery);
				raySamplePdfsRatioQuery = medium->RaySamplePdf(ray, mint, queryIsectDist, 0) / raySamplePdfQuery;
				// Beam
				attenuation *= medium->EvalAttenuation(mRay, 0, beamIsectDist);
				if (mFlags & SHORT_BEAM)
					attenuation /= medium->RaySamplePdf(mRay, 0, beamIsectDist);
				raySamplePdfBeam = medium->RaySamplePdf(mRay, 0, beamIsectDist, mRaySamplingFlags, &raySampleRevPdfBeam);
				raySamplePdfsRatioBeam = medium->RaySamplePdf(mRay, 0, beamIsectDist, 0) / raySamplePdfBeam;
			}
			if (!attenuation.isPositive())
				return;

			// Ray sampling PDFs.
			raySamplePdfQuery *= additionalDataForMis->mRaySamplePdf;
			raySampleRevPdfQuery *= additionalDataForMis->mRaySampleRevPdf;
			raySamplePdfBeam *= mRaySamplePdf;
			raySampleRevPdfBeam *= mRaySampleRevPdf;
			UPBP_ASSERT(raySamplePdfQuery);
			UPBP_ASSERT(raySampleRevPdfQuery);
			UPBP_ASSERT(raySamplePdfBeam);
			UPBP_ASSERT(raySampleRevPdfBeam);			

			// BSDF.
			float bsdfDirPdfW, bsdfRevPdfW;
			const Rgb bsdfFactor = PhaseFunction::Evaluate(-ray.direction, -mRay.direction, medium->MeanCosine(), &bsdfDirPdfW, &bsdfRevPdfW);
			if (bsdfFactor.isBlackOrNegative())
				return;

			bsdfDirPdfW *= medium->ContinuationProb();
			bsdfRevPdfW *= medium->ContinuationProb();
			UPBP_ASSERT(bsdfDirPdfW > 0);			
			UPBP_ASSERT(bsdfRevPdfW > 0);

			// Unweighted result.
			const Rgb unweightedResult =
				(1.0f / beamSelectionPdf) * 
				mThroughputAtOrigin *
				scattering *
				attenuation *
				bsdfFactor *
				(1 - beamBeamDistance * beamBeamDistance / (radius * radius)) * 3 /
				(4 * radius*sinTheta);

			if (unweightedResult.isBlackOrNegative())
				return;

			// Update affected MIS data for query.
			const float distSqQuery = Utils::sqr(queryIsectDist);
			const float raySamplePdfInvQuery = 1.0f / raySamplePdfQuery;
			MisData* cameraVerticesMisData = static_cast<MisData*>(additionalDataForMis->mCameraVerticesMisData);
			cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mPdfAInv = additionalDataForMis->mLastPdfWInv * distSqQuery * raySamplePdfInvQuery;
			//cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mRevPdfA = 1.0f; // not used (sent through AccumulateCameraPathWeight params)
			cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mRaySamplePdfInv = raySamplePdfInvQuery;
			//cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mRaySampleRevPdfInv = 1.0f; // not used (sent through AccumulateCameraPathWeight params)
			cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mRaySamplePdfsRatio = raySamplePdfsRatioQuery;
			//cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mRaySampleRevPdfsRatio = raySamplePdfsRatioBeam; // not used (sent through AccumulateCameraPathWeight params)
			//cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mSinTheta = sinTheta; // not used (sent through AccumulateCameraPathWeight params)
			cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mSurfMisWeightFactor = 0;
			cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mPP3DMisWeightFactor = additionalDataForMis->mPP3DMisWeightFactor;
			cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mPB2DMisWeightFactor = additionalDataForMis->mPB2DMisWeightFactor; 
			cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mBB1DMisWeightFactor = additionalDataForMis->mBB1DMisWeightFactor;
			cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mBB1DBeamSelectionPdf = beamSelectionPdf;
			cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mIsDelta = false;
			cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mIsOnLightSource = false;
			cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mIsSpecular = false;
			cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mInMediumWithBeams = true;

			// Update reverse PDFs of the previous camera vertex.
			//cameraVerticesMisData[additionalDataForMis->mCameraPathLength - 1].mRevPdfA *= raySampleRevPdfQuery; // done directly in AccumulateCameraPathWeight params in order not to spoil the original data (it is not assignement but multiplication!)
			cameraVerticesMisData[additionalDataForMis->mCameraPathLength - 1].mRaySampleRevPdfInv = 1.0f / raySampleRevPdfQuery;

			// Update affected MIS data for beam.
			const float distSqBeam = mLightVertex->mIsFinite ? Utils::sqr(beamIsectDist) : 1.0f;
			const float raySamplePdfInvBeam = 1.0f / raySamplePdfBeam;
			MisData beamLightVertexMisData;
			beamLightVertexMisData.mPdfAInv = mLastPdfWInv * distSqBeam * raySamplePdfInvBeam;
			//beamLightVertexMisData.mRevPdfA = 1.0f;
			//beamLightVertexMisData.mRevPdfAWithoutBsdf = beamLightVertexMisData.mRevPdfA; // not used (sent through AccumulateLightPathWeight params)
			beamLightVertexMisData.mRaySamplePdfInv = raySamplePdfInvBeam;
			//beamLightVertexMisData.mRaySampleRevPdfInv = 1.0f; // not used (sent through AccumulateLightPathWeight params)
			beamLightVertexMisData.mRaySamplePdfsRatio = raySamplePdfsRatioBeam;
			//beamLightVertexMisData.mRaySampleRevPdfsRatio = raySamplePdfsRatioQuery; // not used (sent through AccumulatePathWeight params)
			//beamLightVertexMisData.mSinTheta = sinTheta; // not used (sent through AccumulateLightPathWeight params)
			beamLightVertexMisData.mSurfMisWeightFactor = 0;
			beamLightVertexMisData.mPP3DMisWeightFactor = additionalDataForMis->mPP3DMisWeightFactor;
			beamLightVertexMisData.mPB2DMisWeightFactor = additionalDataForMis->mPB2DMisWeightFactor;
			beamLightVertexMisData.mBB1DMisWeightFactor = additionalDataForMis->mBB1DMisWeightFactor;
			beamLightVertexMisData.mBB1DBeamSelectionPdf = beamSelectionPdf;
			beamLightVertexMisData.mIsDelta = false;
			beamLightVertexMisData.mIsOnLightSource = false;
			beamLightVertexMisData.mIsSpecular = false;
			beamLightVertexMisData.mInMediumWithBeams = true;

			// Update reverse PDFs of the previous light vertex.
			//mLightVertex->mMisData.mRevPdfA *= raySampleRevPdfBeam / distSq; // done directly in AccumulateLightPathWeight params in order not to spoil the original data (it is not assignement but multiplication!)
			//mLightVertex->mMisData.mRevPdfAWithoutBsdf = mLightVertex->mMisData.mRevPdfA; // not used
			const float originRaySampleRevPdfInvBackup = mLightVertex->mMisData.mRaySampleRevPdfInv;
			mLightVertex->mMisData.mRaySampleRevPdfInv = 1.0f / raySampleRevPdfBeam;

			if (rayFlags & NO_SINE_IN_WEIGHTS)
				sinTheta = 1.0;
			
			// Compute MIS weight.
			const float last = (rayFlags & SHORT_BEAM) ?
				((mFlags & SHORT_BEAM) ?
				1.0 / (raySamplePdfsRatioQuery * raySamplePdfsRatioBeam * cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mBB1DMisWeightFactor * beamSelectionPdf * sinTheta)
				: 
				raySamplePdfBeam / (raySamplePdfsRatioQuery * cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mBB1DMisWeightFactor * beamSelectionPdf * sinTheta)) 
				:
				((mFlags & SHORT_BEAM) ?
				raySamplePdfQuery / (raySamplePdfsRatioBeam * cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mBB1DMisWeightFactor * beamSelectionPdf * sinTheta)
				:
				raySamplePdfQuery * raySamplePdfBeam / (cameraVerticesMisData[additionalDataForMis->mCameraPathLength].mBB1DMisWeightFactor * beamSelectionPdf * sinTheta));
			const float wCamera = AccumulateCameraPathWeight(
				additionalDataForMis->mCameraPathLength, 
				last, 
				sinTheta, 
				raySamplePdfInvBeam, 
				raySamplePdfsRatioBeam, 
				bsdfRevPdfW * raySampleRevPdfQuery / distSqQuery, 
				additionalDataForMis->mQueryBeamType, 
				additionalDataForMis->mPhotonBeamType, 
				rayFlags, 
				cameraVerticesMisData);
			const float wLight = AccumulateLightPathWeight(
				mLightVertex->mPathIdx, 
				mLightVertex->mPathLength + 1, 
				last, 
				0, 
				0, 
				0, 
				bsdfDirPdfW * raySampleRevPdfBeam * std::abs(mLightVertex->mMisData.mCosThetaOut) / distSqBeam, 
				BB1D, 
				additionalDataForMis->mQueryBeamType, 
				additionalDataForMis->mPhotonBeamType, 
				rayFlags,
				false,
				static_cast<std::vector<int>*>(additionalDataForMis->mPathEnds), 
				static_cast<std::vector<UPBPLightVertex>*>(additionalDataForMis->mLightVertices),
				&beamLightVertexMisData);
			const float misWeight = 1.f / (wLight + wCamera);

			// Restore modified value of the previous light vertex.
			mLightVertex->mMisData.mRaySampleRevPdfInv = originRaySampleRevPdfInvBackup;

			// Weight and accumulate result.
			accumResult +=
				misWeight *
				unweightedResult;

			UPBP_ASSERT(!accumResult.isNanInfNeg());

			DebugImages & debugImages = *static_cast<DebugImages *>(additionalDataForMis->mDebugImages);
			debugImages.accumRgb2Weight(mLightVertex->mPathLength + 1, DebugImages::BB1D, unweightedResult, misWeight);
		}
	}
};

/**
 * @brief	Defines an alias representing array of photon beams.
 */
typedef std::vector<PhotonBeam> PhotonBeamsArray;

#endif //__PHOTONBEAM_HXX__