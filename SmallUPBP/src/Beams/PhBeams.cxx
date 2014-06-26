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

#include "PhBeams.hxx"
#include "..\Misc\Timer.hxx"
#include "..\Misc\KdTmpl.hxx"

#ifdef USE_BRUTE
#include "PhBrute.hxx"
#endif

#ifdef USE_EMBREE
#include "PhEmbree.hxx"
#endif

#ifdef USE_GRID
#include "PhGrid.hxx"
#endif

/**
 * @brief	Defines an alias representing a kd tree for knn queries.
 */
typedef KdTreeTmplPtr< Pos, Pos > KdTree;

const float MAX_FLOAT_SQUARE_ROOT = std::sqrtf(std::numeric_limits< float >::max()); //!< The maximum float square root

uint PhotonBeamsEvaluator::sGridSize = 256;     //!< Size of the grid.
uint PhotonBeamsEvaluator::sMaxBeamsInCell = 0; //!< Maximum number of tested beams in a single cell. 0 means no restriction.
uint PhotonBeamsEvaluator::sReductionType = 0;  //!< Type of the reduction of numbers of tested beams in cells.	
 
// ----------------------------------------------------------------------------------------------

/**
 * @brief	Builds the data structure for Beam-Beam queries on the given beams.
 *
 * @param [in,out]	beams	 	The beams.
 * @param	radiusCalculation	Type of radius calculation.
 * @param	beamRadius		 	Beam radius.
 * @param	knn				 	Value x means that x-th closest beam vertex will be used for
 * 								calculation of cone radius at the current beam vertex.
 * @param	verbose			 	Whether to print information about progress.
 */
void PhotonBeamsEvaluator::build(PhotonBeamsArray & beams,
	RadiusCalculation radiusCalculation,
	const float beamRadius,
	const int knn,
	bool verbose)
{	
	const float SMALLEST_RADIUS = 0.001f;

	UPBP_ASSERT(accelStruct == nullptr);
	UPBP_ASSERT( !beams.empty() );

	KdTree * tree;
	KdTree::CKNNQuery * query;
	if (radiusCalculation == KNN_RADIUS)
	{
		tree = new KdTree();
		tree->Reserve(beams.size());
		for (int i = 0; i < (int)beams.size(); i++)
		{
			tree->AddItem((Pos *)(&beams[i].mRay.origin), i);
		}
		tree->BuildUp();
		query = new KdTree::CKNNQuery(knn);
	}

	// Define radius of each beam
	for (PhotonBeamsArray::iterator it = beams.begin(); it != beams.end(); ++it)
	{
		if (radiusCalculation == KNN_RADIUS)
		{
			// Query for beam start
			query->Init(it->mRay.origin, knn, MAX_FLOAT_SQUARE_ROOT);
			tree->KNNQuery(*query, tree->truePred);
			UPBP_ASSERT(query->found > 1);
			it->mStartRadius = std::max(2.0f * sqrtf(query->dist2[1]) * beamRadius,SMALLEST_RADIUS);
			// Query for beam end
			query->Init(it->mRay.target(it->mLength), knn, MAX_FLOAT_SQUARE_ROOT);
			tree->KNNQuery(*query, tree->truePred);
			UPBP_ASSERT(query->found > 1);
			it->mEndRadius = std::max(2.0f * sqrtf(query->dist2[1]) * beamRadius, SMALLEST_RADIUS);
			float maxRadius = std::max(it->mStartRadius, it->mEndRadius);
			it->mMaxRadiusSqr = maxRadius * maxRadius;
			it->mRadiusChange = (it->mEndRadius - it->mStartRadius) / it->mLength;
		}
		else
		{			
			it->mStartRadius = it->mEndRadius = beamRadius;
			it->mMaxRadiusSqr = beamRadius * beamRadius;
			it->mRadiusChange = 0.0f;
		}
		it->mInvocation = 0; // Not yet intersected
	}

	if (radiusCalculation == KNN_RADIUS)
	{
		delete query;
		delete tree;
	}

	Timer timer;
	timer.Start();


	accelStruct = new AccelStruct();
	UPBP_ASSERT(accelStruct != nullptr);
#ifdef USE_GRID
	accelStruct->setGridSize(sGridSize);
	accelStruct->setMaxBeamsInCell(sMaxBeamsInCell);
	accelStruct->setReductionType(sReductionType);
	accelStruct->setSeed(mSeed);
#endif
	accelStruct->build(beams, verbose);

	timer.Stop();
	const double treeConstructionTime = timer.GetLastElapsedTime();

	if(verbose) 
	{
		std::cout
			<< std::setprecision(3)
			<< "   - Beam struct took  " << treeConstructionTime << " sec.\n" << std::endl;
	}
}

// ----------------------------------------------------------------------------------------------

/**
 * @brief	Destroys the data structure for Beam-Beam queries.
 */
void PhotonBeamsEvaluator::destroy( ) 
{
	delete accelStruct;
	accelStruct = nullptr;
}

// ----------------------------------------------------------------------------------------------

/**
 * @brief	Evaluates the beam-beam estimate for the given query ray.
 *
 * @param	beamType					   	Type of the beam.
 * @param	queryRay					   	The query ray (=beam) for the Beam-beam estimate.
 * @param	segments					   	Full volume segments of media intersected by the ray.
 * @param	estimatorTechniques			   	The estimator techniques to use.
 * @param	raySamplingFlags			   	The ray sampling flags (\c AbstractMedium::kOriginInMedium).
 * @param [in,out]	additionalRayDataForMis	(Optional) additional data needed for MIS weights
 * 											computations.
 * @param [in,out]	gridStats			   	(Optional) statistics to gather for the ray.
 *
 * @return	The accumulated radiance along the ray.
 */
Rgb PhotonBeamsEvaluator::evalBeamBeamEstimate(
	BeamType beamType,
	const Ray& queryRay,
	const VolumeSegments& segments,
	const uint estimatorTechniques,
	const uint raySamplingFlags,
	embree::AdditionalRayDataForMis* additionalRayDataForMis,
	GridStats* gridStats)
{
	UPBP_ASSERT(estimatorTechniques & BB1D);
	UPBP_ASSERT(raySamplingFlags == 0 || raySamplingFlags == AbstractMedium::kOriginInMedium);
	
	Rgb result(0);

	Rgb attenuation(1);
	float raySamplePdf = 1.0f;
	float raySampleRevPdf = 1.0f;
	GridStats _gridStats;
	if (!gridStats) gridStats = &_gridStats;

	/// Accumulate for each segment
	for (VolumeSegments::const_iterator it = segments.begin(); it != segments.end(); ++it)
	{
		// Get segment medium
		const AbstractMedium * medium = scene.mMedia[it->mMediumID];
		
		// Accumulate
		Rgb segmentResult(0);
		if (medium->HasScattering())
		{
			if (additionalRayDataForMis)
			{
				additionalRayDataForMis->mRaySamplePdf = raySamplePdf;
				additionalRayDataForMis->mRaySampleRevPdf = raySampleRevPdf;
				additionalRayDataForMis->mRaySamplingFlags = AbstractMedium::kEndInMedium;
				if (it == segments.begin())
					additionalRayDataForMis->mRaySamplingFlags |= raySamplingFlags;
			}
			segmentResult = accelStruct->evalBeamBeamEstimate(queryRay, beamType | estimatorTechniques, medium, it->mDistMin, it->mDistMax, *gridStats, additionalRayDataForMis);
		}
		// Add to total result
		result += attenuation * segmentResult;

		if (additionalRayDataForMis)
		{
			DebugImages & debugImages = *static_cast<DebugImages *>(additionalRayDataForMis->mDebugImages);
			debugImages.accumRgb2ToRgb(DebugImages::BB1D, attenuation);
			debugImages.ResetAccum2();
		}

		// Update attenuation
		attenuation *= beamType == SHORT_BEAM ? it->mAttenuation / it->mRaySamplePdf :  // Short beams - no attenuation
			it->mAttenuation;
		if (!attenuation.isPositive())
			return result;

		// Update PDFs
		raySamplePdf *= it->mRaySamplePdf;
		raySampleRevPdf *= it->mRaySampleRevPdf;
	}

	UPBP_ASSERT(!result.isNanInfNeg());

	return result;
}

/**
 * @brief	Evaluates the beam-beam estimate for the given query ray.
 *
 * @param	beamType					   	Type of the beam.
 * @param	queryRay					   	The query ray (=beam) for the Beam-beam estimate.
 * @param	segments					   	Lite volume segments of media intersected by the ray.
 * @param	estimatorTechniques			   	The estimator techniques to use.
 * @param	raySamplingFlags			   	The ray sampling flags (\c AbstractMedium::kOriginInMedium).
 * @param [in,out]	additionalRayDataForMis	(Optional) additional data needed for MIS weights
 * 											computations.
 * @param [in,out]	gridStats			   	(Optional) statistics to gather for the ray.
 *
 * @return	The accumulated radiance along the ray.
 */
Rgb PhotonBeamsEvaluator::evalBeamBeamEstimate(
	BeamType beamType,
	const Ray& queryRay,
	const LiteVolumeSegments& segments,
	const uint estimatorTechniques,
	const uint raySamplingFlags,
	embree::AdditionalRayDataForMis* additionalRayDataForMis,
	GridStats* gridStats)
{
	UPBP_ASSERT(beamType == LONG_BEAM);
	UPBP_ASSERT(estimatorTechniques & BB1D);
	UPBP_ASSERT(raySamplingFlags == 0 || raySamplingFlags == AbstractMedium::kOriginInMedium);

	Rgb result(0);

	Rgb attenuation(1);
	float raySamplePdf = 1.0f;
	float raySampleRevPdf = 1.0f;
	GridStats _gridStats;
	if (!gridStats) gridStats = &_gridStats;

	/// Accumulate for each segment
	for (LiteVolumeSegments::const_iterator it = segments.begin(); it != segments.end(); ++it)
	{
		// Get segment medium
		const AbstractMedium * medium = scene.mMedia[it->mMediumID];

		// Accumulate
		Rgb segmentResult(0);
		if (medium->HasScattering())
		{
			if (additionalRayDataForMis)
			{
				additionalRayDataForMis->mRaySamplePdf = raySamplePdf;
				additionalRayDataForMis->mRaySampleRevPdf = raySampleRevPdf;
				additionalRayDataForMis->mRaySamplingFlags = AbstractMedium::kEndInMedium;
				if (it == segments.begin())
					additionalRayDataForMis->mRaySamplingFlags |= raySamplingFlags;
			}
			segmentResult = accelStruct->evalBeamBeamEstimate(queryRay, beamType | estimatorTechniques, medium, it->mDistMin, it->mDistMax, *gridStats, additionalRayDataForMis);
		}

		// Add to total result
		result += attenuation * segmentResult;

		if (additionalRayDataForMis)
		{
			DebugImages & debugImages = *static_cast<DebugImages *>(additionalRayDataForMis->mDebugImages);
			debugImages.accumRgb2ToRgb(DebugImages::BB1D, attenuation);
			debugImages.ResetAccum2();
		}

		// Update attenuation
		attenuation *= medium->EvalAttenuation(queryRay, it->mDistMin, it->mDistMax);
		if (!attenuation.isPositive())
			return result;

		// Update PDFs
		float segmentRaySampleRevPdf;
		float segmentRaySamplePdf = medium->RaySamplePdf(queryRay, it->mDistMin, it->mDistMax, it == segments.begin() ? raySamplingFlags : 0, &segmentRaySampleRevPdf);
		raySamplePdf *= segmentRaySamplePdf;
		raySampleRevPdf *= segmentRaySampleRevPdf;
	}

	UPBP_ASSERT(!result.isNanInfNeg());

	return result;
}

// ----------------------------------------------------------------------------------------------

/**
 * @brief	Gets probability of selecting a beam (in case of beam reduction) around the given
 * 			position.
 *
 * @param	pos	The position.
 *
 * @return	The beam selection PDF.
 */
float PhotonBeamsEvaluator::getBeamSelectionPdf(const Pos & pos) const
{
	return accelStruct->getBeamSelectionPdf(pos);
}