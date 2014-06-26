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

#ifndef __PATHWEIGHT_HXX__
#define __PATHWEIGHT_HXX__

#include "UPBPLightVertex.hxx"

// Accumulates PDF ratios of all sampling techniques along path originally sampled from camera
static float AccumulateCameraPathWeight(
	const int aPathLength, 
	const float aLastRevPdfA,
	const float aLastSinTheta,
	const float aLastRaySampleRevPdfInv,
	const float aLastRaySampleRevPdfsRatio,
	const float aNextToLastPartialRevPdfW,	
	const uint  aQueryBeamType,
	const uint  aPhotonBeamType,
	const uint  aEstimatorTechniques, 
	const MisData *aCameraVerticesMisData)
{
	float weight = 0;
	float product = 1.0f;
	int lastIndex = aPathLength;
	int index = 0;
	float weightBB1D = 0;

	// No technique for delta vertices
	UPBP_ASSERT(!aCameraVerticesMisData[lastIndex].mIsDelta);

	// Zero vertex (on camera) is ignored (no chance hitting it)
	while (index < aPathLength)
	{
		// Two vertices of the current path segment (same if already at the first camera vertex)
		const MisData& current = aCameraVerticesMisData[lastIndex - index];
		const MisData& next = index < aPathLength - 1 ? aCameraVerticesMisData[lastIndex - index - 1] : current;

		if ((aEstimatorTechniques & BB1D_PREVIOUS) && !current.mIsSpecular)
			weightBB1D = 0;

		// Get reverse data
		float rev = current.mRevPdfA;
		float sinTheta = current.mSinTheta;
		float rayRev = current.mRaySampleRevPdfInv;
		float rayRevRatio = current.mRaySampleRevPdfsRatio;
		if (index == 0)
		{
			rev = aLastRevPdfA;
			sinTheta = aLastSinTheta;
			rayRev = aLastRaySampleRevPdfInv;
			rayRevRatio = aLastRaySampleRevPdfsRatio;
		}
		else if (index == 1)
		{
			rev *= aNextToLastPartialRevPdfW;
		}

		// Reverse probability is never zero
		UPBP_ASSERT(rev);

		product *= rev;

		// Ignore specularity of sampled event for the first vertex (first vertex is never delta here and sampling
		// specular event is a thing of path continuation)
		bool currentUsable = index == 0 || !current.mIsSpecular;

		// SURF
		if ((aEstimatorTechniques & SURF) && currentUsable)
			weight += product * current.mSurfMisWeightFactor;

		// PP3D
		if (aEstimatorTechniques & PP3D)
			weight += product * current.mPP3DMisWeightFactor;

		// PB2D
		if (aEstimatorTechniques & PB2D)
		{
			if (aQueryBeamType & LONG_BEAM)
				weight += product * current.mPB2DMisWeightFactor * current.mRaySamplePdfInv;
			else
				weight += product * current.mPB2DMisWeightFactor * current.mRaySamplePdfsRatio;
		}

		// BB1D
		if (aEstimatorTechniques & BB1D && current.mInMediumWithBeams)
		{
			if (aEstimatorTechniques & NO_SINE_IN_WEIGHTS) sinTheta = 1;
			
			if (aQueryBeamType & LONG_BEAM)
			{
				if (aPhotonBeamType & LONG_BEAM)
					weightBB1D += product * current.mBB1DMisWeightFactor * current.mBB1DBeamSelectionPdf * current.mRaySamplePdfInv * sinTheta * rayRev;
				else
					weightBB1D += product * current.mBB1DMisWeightFactor * current.mBB1DBeamSelectionPdf * current.mRaySamplePdfInv * sinTheta * rayRevRatio;
			}
			else
			{
				if (aPhotonBeamType & LONG_BEAM)
					weightBB1D += product * current.mBB1DMisWeightFactor * current.mBB1DBeamSelectionPdf * current.mRaySamplePdfsRatio * sinTheta * rayRev;
				else
					weightBB1D += product * current.mBB1DMisWeightFactor * current.mBB1DBeamSelectionPdf * current.mRaySamplePdfsRatio * sinTheta * rayRevRatio;
			}
		}

		// Get inverse of forward PDF
		float fwInv = current.mPdfAInv;
		UPBP_ASSERT(fwInv);

		product *= fwInv;

		// BPT
		if ((aEstimatorTechniques & BPT) && currentUsable && !next.mIsSpecular)
			weight += product;

		++index;
	}

	return weight + weightBB1D;
}

// Accumulates PDF ratios of all sampling techniques along path originally sampled from light
static float AccumulateLightPathWeight(
	const int   aPathIndex,
	const int   aPathLength,
	const float aLastRevPdfA,
	const float aLastSinTheta,
	const float aLastRaySampleRevPdfInv,
	const float aLastRaySampleRevPdfsRatio,
	const float aNextToLastPartialRevPdfW,
	const uint  aCurrentlyEvaluatedTechnique,
	const uint  aQueryBeamType,
	const uint  aPhotonBeamType,
	const uint  aEstimatorTechniques,
	const bool  aCameraConnection,
	const std::vector<int> *aPathEnds,
	const std::vector<UPBPLightVertex> *aLightVertices,
	const MisData* aBeamLightVertexMisData = NULL)
{
	UPBP_ASSERT(aCurrentlyEvaluatedTechnique == BPT || aCurrentlyEvaluatedTechnique == SURF || aCurrentlyEvaluatedTechnique == PP3D || aCurrentlyEvaluatedTechnique == PB2D || aCurrentlyEvaluatedTechnique == BB1D);
	UPBP_ASSERT(aCurrentlyEvaluatedTechnique != BB1D || aBeamLightVertexMisData);

	float weight = 0;
	float product = 1.0f;
	int lastIndex = (aPathIndex == 0) ? aPathLength : aPathEnds->at(aPathIndex - 1) + aPathLength;
	int index = 0;

	// No technique for delta vertices
	UPBP_ASSERT((aCurrentlyEvaluatedTechnique == BB1D && !aBeamLightVertexMisData->mIsDelta) || !aLightVertices->at(lastIndex).mMisData.mIsDelta);

	while (index <= aPathLength)
	{
		// Two vertices of the current path segment (same if already at light)
		const MisData& current = (aCurrentlyEvaluatedTechnique == BB1D && index == 0) ? *aBeamLightVertexMisData : aLightVertices->at(lastIndex - index).mMisData;
		const MisData& next = index < aPathLength ? aLightVertices->at(lastIndex - index - 1).mMisData : current;

		// Get reverse data
		float rev = current.mRevPdfA;
		float sinTheta = current.mSinTheta;
		float rayRev = current.mRaySampleRevPdfInv;
		float rayRevRatio = current.mRaySampleRevPdfsRatio;
		if (index == 0)
		{
			rev = aLastRevPdfA;
			sinTheta = aLastSinTheta;
			rayRev = aLastRaySampleRevPdfInv;
			rayRevRatio = aLastRaySampleRevPdfsRatio;
		}
		else if (index == 1)
		{
			if (aCurrentlyEvaluatedTechnique == BB1D)
				rev = aNextToLastPartialRevPdfW;
			else
				rev = current.mRevPdfAWithoutBsdf * aNextToLastPartialRevPdfW;
		}

		// Reverse probability is never zero unless we are on delta light (such vertex is never the first one since there is no merging, 
		// connection or camera sampling for on-light vertices and light sampling does not call this method)
		UPBP_ASSERT(rev || (current.mIsOnLightSource && current.mIsDelta));

		if (rev == 0)
			break;

		product *= rev;

		// Ignore specularity of sampled event for the first vertex (first vertex is never delta here and sampling
		// specular event is a thing of path continuation)
		bool currentUsable = index == 0 || !current.mIsSpecular;

		// For vertex merging techniques (SURF, PP3D, PB2D, BB1D) merging weights of the first vertex are computed on camera part of the path
		if (index != 0 || aCurrentlyEvaluatedTechnique == BPT)
		{
			// SURF
			if ((aEstimatorTechniques & SURF) && currentUsable)
				weight += product * current.mSurfMisWeightFactor;

			// PP3D
			if (aEstimatorTechniques & PP3D)
				weight += product * current.mPP3DMisWeightFactor;

			// PB2D
			if (aEstimatorTechniques & PB2D)
			{
				if (aQueryBeamType & LONG_BEAM)
					weight += product * current.mPB2DMisWeightFactor * rayRev;
				else
					weight += product * current.mPB2DMisWeightFactor * rayRevRatio;
			}

			// BB1D
			if ((aEstimatorTechniques & BB1D) && (!(aEstimatorTechniques & BB1D_PREVIOUS) || (aCameraConnection && index == 0)) && current.mInMediumWithBeams)
			{
				if (aEstimatorTechniques & NO_SINE_IN_WEIGHTS) sinTheta = 1;
				
				if (aQueryBeamType & LONG_BEAM)
				{
					if (aPhotonBeamType & LONG_BEAM)
						weight += product * current.mBB1DMisWeightFactor * current.mBB1DBeamSelectionPdf * rayRev * sinTheta * current.mRaySamplePdfInv;
					else
						weight += product * current.mBB1DMisWeightFactor * current.mBB1DBeamSelectionPdf * rayRev * sinTheta * current.mRaySamplePdfsRatio;
				}
				else
				{
					if (aPhotonBeamType & LONG_BEAM)
						weight += product * current.mBB1DMisWeightFactor * current.mBB1DBeamSelectionPdf * rayRevRatio * sinTheta * current.mRaySamplePdfInv;
					else
						weight += product * current.mBB1DMisWeightFactor * current.mBB1DBeamSelectionPdf * rayRevRatio * sinTheta * current.mRaySamplePdfsRatio;
				}
			}
		}

		// Get inverse of forward PDF
		float fwInv = current.mPdfAInv;
		UPBP_ASSERT(fwInv);

		product *= fwInv;

		// BPT
		if ((aEstimatorTechniques & BPT) && currentUsable && !next.mIsSpecular)
			weight += product;

		++index;

		if (aEstimatorTechniques & PREVIOUS) break;
	}

	return weight;
}

#endif //__PATHWEIGHT_HXX__