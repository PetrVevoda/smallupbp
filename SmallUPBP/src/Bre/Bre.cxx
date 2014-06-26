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

// embree includes
#include "include\embree.h"
#include "common\ray.h"

#include "Bre.hxx"
#include "..\Beams\PhBeams.hxx"
#include "..\Misc\Timer.hxx"
#include "..\Misc\KdTmpl.hxx"
#include "..\Misc\DebugImages.hxx"
#include "..\Path\PathWeight.hxx"
#include "..\Structs\BoundingBox.hxx"

/**
 * @brief	Defines an alias representing the kd tree.
 */
typedef KdTreeTmplPtr< Pos,Pos > KdTree;

const float MAX_FLOAT_SQUARE_ROOT = std::sqrtf(std::numeric_limits< float >::max());	//!< The maximum float square root

// ----------------------------------------------------------------------------------------------

/**
 * @brief	Converts the \c Pos to \c embree::Vector3f.
 *
 * @param	pos	The position.
 *
 * @return	Pos as an embree::Vector3f.
 */
INLINE embree::Vector3f toEmbreeV3f (const Pos& pos)
{
	return embree::Vector3f(pos.x(), pos.y(), pos.z());
}

// ----------------------------------------------------------------------------------------------

/**
 * @brief	Converts the \c Dir to \c embree::Vector3f.
 *
 * @param	dir	The direction.
 *
 * @return	Dir as an embree::Vector3f.
 */
INLINE embree::Vector3f toEmbreeV3f (const Dir& dir)
{
	return embree::Vector3f(dir.x(), dir.y(), dir.z());
}

// ----------------------------------------------------------------------------------------------

INLINE Pos toPos(const embree::Vector3f& pos)
{
	return Pos(pos.x, pos.y, pos.z);
}

// ----------------------------------------------------------------------------------------------

/**
 * @brief	Converts \c embree::Vector3f to \c Pos.
 *
 * @param	dir	The direction.
 *
 * @return	embree::Vector3f as a Dir.
 */
INLINE Dir toDir(const embree::Vector3f& dir)
{
	return Dir(dir.x, dir.y, dir.z);
}

// ----------------------------------------------------------------------------------------------

/**
 * @brief	An embree photon.
 */
class EmbreePhoton : public embree::Intersector1
{
public:
	Pos    pos;       //!< The position.
	float  radius;    //!< The radius.
	float  radiusSqr; //!< The radius squared.
	Dir    incDir;    //!< The incoming direction.
	Rgb    flux;	  //!< The flux.
	const UPBPLightVertex *lightVertex; //!< The corresponding light vertex.

	/**
	 * @brief	Default constructor.
	 */
	EmbreePhoton() {}

	/**
	 * @brief	Constructor.
	 *
	 * @param	aPos   	The position.
	 * @param	aRadius	The radius.
	 * @param	aIncDir	The incoming direction.
	 * @param	aFlux  	The flux.
	 */
	EmbreePhoton(const Pos& aPos, const float aRadius, const Dir& aIncDir, const Rgb& aFlux )
		: Intersector1( breIntersectFuncHomogeneous, breOccludedFunc )
		, pos(aPos)
		, radius(aRadius)
		, radiusSqr(aRadius*aRadius)
		, incDir(aIncDir)
		, flux(aFlux)
	{

	}

	/**
	 * @brief	Sets photon properties and function pointers.
	 *
	 * @param	aPos   	The position.
	 * @param	aRadius	The radius.
	 * @param	aIncDir	The incoming direction.
	 * @param	aFlux  	The flux.
	 */
	void set(const Pos& aPos, const float aRadius, const Dir& aIncDir, const Rgb& aFlux)
	{
		intersectPtr = breIntersectFuncHomogeneous;
		occludedPtr = breOccludedFunc;
		pos = aPos;
		radius = aRadius;
		radiusSqr = aRadius*aRadius;
		incDir = aIncDir;
		flux = aFlux;
	}

	/**
	 * @brief	Sets photon properties and function pointers.
	 *
	 * @param	aRadius			The radius.
	 * @param	aLightVertex	The light vertex corresponding to the photon.
	 */
	void set(const float aRadius, const UPBPLightVertex * aLightVertex)
	{
		intersectPtr = breIntersectFuncHomogeneous2;
		occludedPtr = breOccludedFunc;
		pos = aLightVertex->mHitpoint;
		radius = aRadius;
		radiusSqr = aRadius*aRadius;
		lightVertex = aLightVertex;
	}

	/**
	 * @brief	Gets the bounding box.
	 *
	 * @return	The bounding box.
	 */
	BoundingBox3 getBbox()
	{
		return BoundingBox3( pos - Dir(radius), pos + Dir(radius) );
	}

	/**
	 * @brief	Test intersection between a ray and a 'photon disc'.
	 * 			
	 * 			The photon disc is specified by the position aPhotonPos and radius aPhotonRad. The
	 * 			disk is assumed to face the ray (i.e. the disk plane is perpendicular to the ray).
	 * 			Intersections are reported only in the interval [aMinT, aMaxT)  (i.e. includes aMinT
	 * 			but excludes aMaxT).
	 *
	 * @param [in,out]	oIsectDist  	The intersection distance along the ray.
	 * @param [in,out]	oIsectRadSqr	The square of the distance of the intersection point from the photon location.
	 * @param	aQueryRay				The query ray.
	 * @param	aMinT					The minimum t.
	 * @param	aMaxT					The maximum t.
	 * @param	aPhotonPos				The photon position.
	 * @param	aPhotonRadSqr			The photon radius.
	 *
	 * @return	true is an intersection is found, false otherwise. If an intersection is found,
	 * 			oIsectDist is set to the intersection distance along the ray, and oIsectRadSqr is set
	 * 			to the square of the distance of the intersection point from the photon location.
	 */
	static INLINE bool testIntersectionBre(
		float& oIsectDist,
		float& oIsectRadSqr,
		const Ray &aQueryRay, 
		const float aMinT,
		const float aMaxT,
		const Pos &aPhotonPos, 
		const float aPhotonRadSqr)
	{
		const Dir rayOrigToPhoton = aPhotonPos - aQueryRay.origin;

		const float isectDist = dot(rayOrigToPhoton, aQueryRay.direction);

		if ( isectDist > aMinT && isectDist < aMaxT )
		{
			const float isectRadSqr = (aQueryRay.origin + aQueryRay.direction * isectDist - aPhotonPos).square();
			if ( isectRadSqr <= aPhotonRadSqr )
			{
				oIsectDist   = isectDist;
				oIsectRadSqr = isectRadSqr;
				return true;
			}
		}
		return false;
	}

	/**
	 * @brief	BRE intersection function for a photon.
	 * 			
	 * 			Version used by pure PB2D algorithm from \c VolLightTracer.hxx.
	 *
	 * @param	This	   	EmbreePhoton that we are testing intersection with.
	 * @param [in,out]	ray	BRE query ray.
	 *
	 * @return	If no intersection is found, ray is left unchanged. To report an intersection
	 * 			back to embree, one needs to set ray.tfar to the intersection distance and
	 * 			ray.id0 and ray.id1 to the id of the intersected object. In the BRE query we
	 * 			never report intersections to embree because we want to keep traversing the data
	 * 			structure.
	 */
	static void breIntersectFuncHomogeneous(const embree::Intersector1* This, embree::Ray& ray)
	{		
		const EmbreePhoton* thisPhoton = (const EmbreePhoton*)This;
		
		float photonIsectDist, isectRadSqr;

		if (testIntersectionBre(photonIsectDist, isectRadSqr, *ray.origRay, ray.tnear, ray.tfar, thisPhoton->pos, thisPhoton->radiusSqr))
		{			
			// Found an intersection.
			const Pos isectPt = ray.origRay->origin + photonIsectDist * ray.origRay->direction;

			const Rgb& scatteringCoeff = ray.medium->IsHomogeneous() ? ((const HomogeneousMedium *)ray.medium)->GetScatteringCoef() : ray.medium->GetScatteringCoef(isectPt);

			Rgb attenuation;
			if (ray.medium->IsHomogeneous())
			{
				const HomogeneousMedium * medium = ((const HomogeneousMedium *)ray.medium);
				attenuation = medium->EvalAttenuation(photonIsectDist - ray.tnear);
				if (ray.flags & SHORT_BEAM)
					attenuation /= attenuation[medium->mMinPositiveAttenuationCoefCoordIndex()];
			}
			else
			{
				attenuation = ray.medium->EvalAttenuation(*ray.origRay, ray.tnear, photonIsectDist);
				if (ray.flags & SHORT_BEAM)
					attenuation /= ray.medium->RaySamplePdf(*ray.origRay, ray.tnear, photonIsectDist);
			}
			
			*static_cast<Rgb*>(ray.accumResult) +=
				thisPhoton->flux *
				attenuation *
				scatteringCoeff *
				PhaseFunction::Evaluate(ray.origRay->direction, -thisPhoton->incDir, ray.medium->MeanCosine()) *
				// Epanechnikov kernel
				(1 - isectRadSqr / thisPhoton->radiusSqr) / (thisPhoton->radiusSqr * PI_F * 0.5f);
			
			//UPBP_ASSERT((1 - isectRadSqr) / (PI_F * (thisPhoton->radiusSqr - thisPhoton->radiusSqr * thisPhoton->radiusSqr * 0.5f)) > 0.0f);
		}
	}

	/**
	 * @brief	BRE intersection function for a photon.
	 * 			
	 * 			Version used by combined PB2D algorithms from \c UPBP.hxx.
	 *
	 * @param	This	   	EmbreePhoton that we are testing intersection with.
	 * @param [in,out]	ray	BRE query ray.
	 *
	 * @return	If no intersection is found, ray is left unchanged. To report an intersection
	 * 			back to embree, one needs to set ray.tfar to the intersection distance and
	 * 			ray.id0 and ray.id1 to the id of the intersected object. In the BRE query we
	 * 			never report intersections to embree because we want to keep traversing the data
	 * 			structure.
	 */
	static void breIntersectFuncHomogeneous2(const embree::Intersector1* This, embree::Ray& ray)
	{		
		const EmbreePhoton* thisPhoton = (const EmbreePhoton*)This;
		const UPBPLightVertex* lightVertex = thisPhoton->lightVertex;
		const embree::AdditionalRayDataForMis* data = ray.additionalRayDataForMis;

		UPBP_ASSERT(lightVertex);
		UPBP_ASSERT(lightVertex->mInMedium);
		UPBP_ASSERT(data);

		float photonIsectDist, isectRadSqr;

		if (testIntersectionBre(photonIsectDist, isectRadSqr, *ray.origRay, ray.tnear, ray.tfar, lightVertex->mHitpoint, thisPhoton->radiusSqr))
		{			
			UPBP_ASSERT(photonIsectDist);

			// Heterogeneous medium is not supported.
			UPBP_ASSERT(ray.medium->IsHomogeneous());
			
			// Reject if full path length below/above min/max path length.
			if ((lightVertex->mPathLength + data->mCameraPathLength > data->mMaxPathLength) ||
				(lightVertex->mPathLength + data->mCameraPathLength < data->mMinPathLength))
			return;

			// Ignore contribution of primary rays from medium too close to camera.
			if (data->mCameraPathLength == 1 && photonIsectDist < data->mMinDistToMed)
				return;
		
			// Compute intersection.
			const Pos isectPt = ray.origRay->origin + photonIsectDist * ray.origRay->direction;

			// Compute attenuation in current segment and overall pds.
			Rgb attenuation;
			float raySamplePdf = 1.0f;
			float raySampleRevPdf = 1.0f;
			float raySamplePdfsRatio = 1.0f;
			if (ray.medium->IsHomogeneous())
			{
				const HomogeneousMedium * medium = ((const HomogeneousMedium *)ray.medium);
				attenuation = medium->EvalAttenuation(photonIsectDist - ray.tnear);
				const float pfd = attenuation[medium->mMinPositiveAttenuationCoefCoordIndex()];
				if (ray.flags & SHORT_BEAM)
					attenuation /= pfd;
				raySamplePdf = medium->mMinPositiveAttenuationCoefCoord() * pfd;
				raySampleRevPdf = (data->mRaySamplingFlags & AbstractMedium::kOriginInMedium) ? raySamplePdf : pfd;
				raySamplePdfsRatio = 1.0f / medium->mMinPositiveAttenuationCoefCoord();
			}
			else
			{
				attenuation = ray.medium->EvalAttenuation(*ray.origRay, ray.tnear, photonIsectDist);
				if (ray.flags & SHORT_BEAM)
					attenuation /= ray.medium->RaySamplePdf(*ray.origRay, ray.tnear, photonIsectDist);
				raySamplePdf = ray.medium->RaySamplePdf(*ray.origRay, ray.tnear, photonIsectDist, data->mRaySamplingFlags, &raySampleRevPdf);
				raySamplePdfsRatio = ray.medium->RaySamplePdf(*ray.origRay, ray.tnear, photonIsectDist, 0) / raySamplePdf;
			}			
			if (!attenuation.isPositive())
				return;
			
			raySamplePdf *= data->mRaySamplePdf;
			raySampleRevPdf *= data->mRaySampleRevPdf;
			UPBP_ASSERT(raySamplePdf);
			UPBP_ASSERT(raySampleRevPdf);

			// Retrieve light incoming direction in world coordinates.
			const Dir lightDirection = lightVertex->mBSDF.WorldDirFix();

			// BSDF.
			float cameraBsdfDirPdfW, cameraBsdfRevPdfW, sinTheta;
			const Rgb cameraBsdfFactor = PhaseFunction::Evaluate(-(*((Dir*)&ray.dir)), lightDirection, ray.medium->MeanCosine(), &cameraBsdfDirPdfW, &cameraBsdfRevPdfW, &sinTheta);
			if (cameraBsdfFactor.isBlackOrNegative())
				return;

			cameraBsdfDirPdfW *= ray.medium->ContinuationProb();
			UPBP_ASSERT(cameraBsdfDirPdfW > 0);

			// Even though this is PDF from camera BSDF, the continuation probability
			// must come from light BSDF, because that would govern it if light path
			// actually continued.
			cameraBsdfRevPdfW *= lightVertex->mBSDF.ContinuationProb();
			UPBP_ASSERT(cameraBsdfRevPdfW > 0);

			// Epanechnikov kernel.
			const float kernel = (1 - isectRadSqr / thisPhoton->radiusSqr) / (thisPhoton->radiusSqr * PI_F * 0.5f);
			if (!Float::isPositive(kernel))
				return;

			// Scattering coefficient.
			const Rgb& scatteringCoeff = ray.medium->IsHomogeneous() ? ((const HomogeneousMedium *)ray.medium)->GetScatteringCoef() : ray.medium->GetScatteringCoef(isectPt);

			// Unweighted result.
			const Rgb unweightedResult = lightVertex->mThroughput *
				attenuation *
				scatteringCoeff *
				cameraBsdfFactor *
				kernel;

			if (unweightedResult.isBlackOrNegative())
				return;

			// Update affected MIS data.
			const float distSq = Utils::sqr(photonIsectDist);
			const float raySamplePdfInv = 1.0f / raySamplePdf;
			MisData* cameraVerticesMisData = static_cast<MisData*>(data->mCameraVerticesMisData);
			cameraVerticesMisData[data->mCameraPathLength].mPdfAInv = data->mLastPdfWInv * distSq * raySamplePdfInv;
			//cameraVerticesMisData[data->mCameraPathLength].mRevPdfA = 1.0f; // not used (sent through AccumulateCameraPathWeight params)
			cameraVerticesMisData[data->mCameraPathLength].mRaySamplePdfInv = raySamplePdfInv;
			//cameraVerticesMisData[data->mCameraPathLength].mRaySampleRevPdfInv = lightVertex->mMisData.mRaySamplePdfInv; // not used (sent through AccumulateCameraPathWeight params)
			cameraVerticesMisData[data->mCameraPathLength].mRaySamplePdfsRatio = raySamplePdfsRatio;
			//cameraVerticesMisData[data->mCameraPathLength].mRaySampleRevPdfsRatio = lightVertex->mMisData.mRaySamplePdfsRatio; // not used (sent through AccumulateCameraPathWeight params)
			//cameraVerticesMisData[data->mCameraPathLength].mSinTheta = sinTheta; // not used (sent through AccumulateCameraPathWeight params)
			cameraVerticesMisData[data->mCameraPathLength].mSurfMisWeightFactor = 0;
			cameraVerticesMisData[data->mCameraPathLength].mPP3DMisWeightFactor = data->mPP3DMisWeightFactor;
			cameraVerticesMisData[data->mCameraPathLength].mPB2DMisWeightFactor = data->mPB2DMisWeightFactor; //data->mLightSubPathCount / kernel;
			cameraVerticesMisData[data->mCameraPathLength].mBB1DMisWeightFactor = data->mBB1DMisWeightFactor;
			if (!data->mBB1DPhotonBeams) cameraVerticesMisData[data->mCameraPathLength].mBB1DBeamSelectionPdf = 0.0f;
			else
			{
				PhotonBeamsEvaluator* pbe = static_cast<PhotonBeamsEvaluator*>(data->mBB1DPhotonBeams);
				if (pbe->sMaxBeamsInCell) 
					cameraVerticesMisData[data->mCameraPathLength].mBB1DBeamSelectionPdf = pbe->getBeamSelectionPdf(isectPt);
				else
					cameraVerticesMisData[data->mCameraPathLength].mBB1DBeamSelectionPdf = 1.0f;								
			}
			cameraVerticesMisData[data->mCameraPathLength].mIsDelta = false;
			cameraVerticesMisData[data->mCameraPathLength].mIsOnLightSource = false;
			cameraVerticesMisData[data->mCameraPathLength].mIsSpecular = false;
			cameraVerticesMisData[data->mCameraPathLength].mInMediumWithBeams = ray.medium->GetMeanFreePath(isectPt) > data->mBB1DMinMFP;

			// Update reverse PDFs of the previous vertex.
			cameraVerticesMisData[data->mCameraPathLength - 1].mRaySampleRevPdfInv = 1.0f / raySampleRevPdf;

			// Compute MIS weight.
			const float last = (ray.flags & SHORT_BEAM) ? 
				1.0 / (raySamplePdfsRatio * cameraVerticesMisData[data->mCameraPathLength].mPB2DMisWeightFactor) : 
				raySamplePdf / cameraVerticesMisData[data->mCameraPathLength].mPB2DMisWeightFactor;
			const float wCamera = AccumulateCameraPathWeight(
				data->mCameraPathLength, 
				last, 
				sinTheta, 
				lightVertex->mMisData.mRaySamplePdfInv, 
				lightVertex->mMisData.mRaySamplePdfsRatio, 
				cameraBsdfRevPdfW * raySampleRevPdf / distSq, 
				data->mQueryBeamType, 
				data->mPhotonBeamType, 
				ray.flags, 
				cameraVerticesMisData);
			const float wLight = AccumulateLightPathWeight(
				lightVertex->mPathIdx, 
				lightVertex->mPathLength, 
				last, 
				0, 
				0, 
				0, 
				cameraBsdfDirPdfW, 
				PB2D, 
				data->mQueryBeamType, 
				data->mPhotonBeamType,
				ray.flags,
				false,
				static_cast<std::vector<int>*>(data->mPathEnds), 
				static_cast<std::vector<UPBPLightVertex>*>(data->mLightVertices));
			const float misWeight = 1.f / (wLight + wCamera);			

			// Weight and accumulate contribution.
			*static_cast<Rgb*>(ray.accumResult) +=
				misWeight *
				unweightedResult;

			DebugImages & debugImages = *static_cast<DebugImages *>(data->mDebugImages);
			debugImages.accumRgb2Weight(lightVertex->mPathLength, DebugImages::PB2D, unweightedResult, misWeight);
		}
	}

	/**
	 * @brief	BRE intersection function for a photon - should never be called.
	 *
	 * @param	This	   	This.
	 * @param [in,out]	ray	The ray.
	 *
	 * @return	Never returns.
	 */
	static bool breOccludedFunc(const embree::Intersector1* This, embree::Ray& ray)
	{
		std::cerr << "Error: breOccludedFunc() called - makes not sense" << std::endl;
		exit(2);
	}
};


// ----------------------------------------------------------------------------------------------

/**
 * @brief	Build a structure of 'virtual' objects - i.e. photon spheres.
 *
 * @param [in,out]	photons	The photons.
 * @param	numPhotons	   	Number of photons.
 *
 * @return	The structure of photon spheres.
 */
static embree::RTCGeometry* buildPhotonTree(EmbreePhoton *photons, int numPhotons) 
{
    embree::RTCGeometry* embreeGeo = embree::rtcNewVirtualGeometry(numPhotons, "default");
    
    for(int i = 0; i < numPhotons; ++i) 
	{
        BoundingBox3 bbox = photons[i].getBbox();
        embree::rtcSetVirtualGeometryUserData(embreeGeo, i, i, 0);
        embree::rtcSetVirtualGeometryBounds(embreeGeo, i, &bbox.point1.x(), &bbox.point2.x());
        embree::rtcSetVirtualGeometryIntersector1 (embreeGeo, i, &photons[i]);
    }
  
    embree::rtcBuildAccel(embreeGeo, "default"); 
    embree::rtcCleanupGeometry(embreeGeo);
    return embreeGeo;
}

// ----------------------------------------------------------------------------------------------

/**
 * @brief	Build the data structure for BRE queries.
 * 			
 * 			Version used by pure PB2D algorithm from \c VolLightTracer.hxx.
 *
 * @param	lightSubPathVertices	Light sub path vertices.
 * @param	numVertices				Number of vertices.
 * @param	radiusCalculation   	Type of radius calculation.
 * @param	photonRadius			Photon radius.
 * @param	knn						Value x means that x-th closest photon will be used for
 * 									calculation of radius of the current photon.
 * @param	verbose					Whether to print information about progress.
 *
 * @return	Number of photons made from the given light sub path vertices.
 */
int EmbreeBre::build(const VltLightVertex* lightSubPathVertices, const int numVertices, RadiusCalculation radiusCalculation, const float photonRadius,
	const int knn, bool verbose)
{
	UPBP_ASSERT( embreePhotons == nullptr );
	UPBP_ASSERT( numEmbreePhotons == 0 );
	UPBP_ASSERT( embreeGeo == nullptr );
	UPBP_ASSERT( embreeIntersector == nullptr );
	UPBP_ASSERT( lightSubPathVertices != nullptr );
	UPBP_ASSERT( numVertices > 0 );
	UPBP_ASSERT(radiusCalculation == CONSTANT_RADIUS || knn > 0);

	// Count number of vertices in medium.
	int numVerticesInMedium = 0;
	for ( int i=0; i<numVertices; i++ )
	{
		if( lightSubPathVertices[i].mIsInMedium )
			numVerticesInMedium ++;
	}

	// Nothing to do.
	if( numVerticesInMedium <= 0 )
		return numVerticesInMedium;

	if(verbose)
		std::cout << " + BRE data struct construction over " << numVerticesInMedium << " photons..." << std::endl;

	Timer timer;
	timer.Start();

	// Allocate embree photons.
	embreePhotons = new EmbreePhoton[numVerticesInMedium];
	numEmbreePhotons = numVerticesInMedium;

	UPBP_ASSERT( embreePhotons != nullptr );

	
	KdTree * tree;
	KdTree::CKNNQuery * query;
	if (radiusCalculation == KNN_RADIUS)
	{
		tree = new KdTree();
		tree->Reserve(numVerticesInMedium);
		for (int i = 0; i < numVertices; i++)
		{
			if (lightSubPathVertices[i].mIsInMedium)
			{
				tree->AddItem((Pos *)(&lightSubPathVertices[i].mHitpoint), i);
			}
		}
		tree->BuildUp();
		query = new KdTree::CKNNQuery(knn);
	}
	// Convert path vertices to embree photons.
	int inMediumIdx = 0;
	for(int i=0; i<numVertices; i++)
	{
		if( lightSubPathVertices[i].mIsInMedium )
		{
			const VltLightVertex& v = lightSubPathVertices[i];
			float radius = photonRadius;
			if (radiusCalculation == KNN_RADIUS)
			{
				query->Init(v.mHitpoint, knn, MAX_FLOAT_SQUARE_ROOT);
				// Execute query.
				tree->KNNQuery(*query, tree->truePred);
				UPBP_ASSERT(query->found > 1);
				radius *= 2.0f * sqrtf(query->dist2[1]);
			}
			embreePhotons[inMediumIdx].set( v.mHitpoint, radius, v.mBSDF.WorldDirFix(), v.mThroughput );
			inMediumIdx++;
		}
	}
	if (radiusCalculation == KNN_RADIUS)
	{
		delete query;
		delete tree;
	}
	UPBP_ASSERT( inMediumIdx == numVerticesInMedium );

	timer.Stop();
	const double dataCopnversionTime = timer.GetLastElapsedTime();
	timer.Start();

	// Build embree data structure.
	embreeGeo = buildPhotonTree( embreePhotons, numVerticesInMedium );

	UPBP_ASSERT( embreeGeo != nullptr );

	// Retrieve the intersectable interface for this data structure.
	embreeIntersector = embree::rtcQueryIntersector1 ( embreeGeo, "default" );

	UPBP_ASSERT( embreeIntersector != nullptr );

	timer.Stop();
	const double treeConstructionTime = timer.GetLastElapsedTime();

	if(verbose) 
	{
		std::cout 
			<< std::setprecision(3)
			<< "   - BRE struct took  " << dataCopnversionTime+treeConstructionTime << " sec.\n"
			<< "       data conversion:   " << dataCopnversionTime  << " sec.\n"
			<< "       tree construction: " << treeConstructionTime << " sec." << std::endl;
	}

	return numVerticesInMedium;
}

/**
 * @brief	Build the data structure for BRE queries.
 * 			
 * 			Version used by combined PB2D algorithms from \c UPBP.hxx.
 *
 * @param	lightSubPathVertices	Light sub path vertices.
 * @param	numVertices				Number of vertices.
 * @param	radiusCalculation   	Type of radius calculation.
 * @param	photonRadius			Photon radius.
 * @param	knn						Value x means that x-th closest photon will be used for
 * 									calculation of radius of the current photon.
 * @param	verbose					Whether to print information about progress.
 *
 * @return	Number of photons made from the given light sub path vertices.
 */
int EmbreeBre::build(const UPBPLightVertex* lightSubPathVertices, const int numVertices, RadiusCalculation radiusCalculation, const float photonRadius,
	const int knn, bool verbose)
{
	UPBP_ASSERT(embreePhotons == nullptr);
	UPBP_ASSERT(numEmbreePhotons == 0);
	UPBP_ASSERT(embreeGeo == nullptr);
	UPBP_ASSERT(embreeIntersector == nullptr);
	UPBP_ASSERT(lightSubPathVertices != nullptr);
	UPBP_ASSERT(numVertices > 0);
	UPBP_ASSERT(radiusCalculation == CONSTANT_RADIUS || knn > 0);

	// Count number of vertices in medium.
	int numVerticesInMedium = 0;
	for (int i = 0; i<numVertices; i++)
	{
		if (lightSubPathVertices[i].mInMedium)
			numVerticesInMedium++;
	}

	// Nothing to do.
	if (numVerticesInMedium <= 0)
		return numVerticesInMedium;

	if (verbose)
		std::cout << " + BRE data struct construction over " << numVerticesInMedium << " photons..." << std::endl;

	Timer timer;
	timer.Start();

	// Allocate embree photons.
	embreePhotons = new EmbreePhoton[numVerticesInMedium];
	numEmbreePhotons = numVerticesInMedium;

	UPBP_ASSERT(embreePhotons != nullptr);


	KdTree * tree;
	KdTree::CKNNQuery * query;
	if (radiusCalculation == KNN_RADIUS)
	{
		tree = new KdTree();
		tree->Reserve(numVerticesInMedium);
		for (int i = 0; i < numVertices; i++)
		{
			if (lightSubPathVertices[i].mInMedium)
			{
				tree->AddItem((Pos *)(&lightSubPathVertices[i].mHitpoint), i);
			}
		}
		tree->BuildUp();
		query = new KdTree::CKNNQuery(knn);
	}
	// Convert path vertices to embree photons.
	int inMediumIdx = 0;
	for (int i = 0; i<numVertices; i++)
	{
		if (lightSubPathVertices[i].mInMedium)
		{
			const UPBPLightVertex& v = lightSubPathVertices[i];
			float radius = photonRadius;
			if (radiusCalculation == KNN_RADIUS)
			{
				query->Init(v.mHitpoint, knn, MAX_FLOAT_SQUARE_ROOT);
				// Execute query.
				tree->KNNQuery(*query, tree->truePred);
				UPBP_ASSERT(query->found > 1);
				radius *= 2.0f * sqrtf(query->dist2[1]);
			}
			embreePhotons[inMediumIdx].set(radius, &v);
			inMediumIdx++;
		}
	}
	if (radiusCalculation == KNN_RADIUS)
	{
		delete query;
		delete tree;
	}
	UPBP_ASSERT(inMediumIdx == numVerticesInMedium);

	timer.Stop();
	const double dataCopnversionTime = timer.GetLastElapsedTime();
	timer.Start();

	// Build embree data structure.
	embreeGeo = buildPhotonTree(embreePhotons, numVerticesInMedium);

	UPBP_ASSERT(embreeGeo != nullptr);

	// Retrieve the intersectable interface for this data structure.
	embreeIntersector = embree::rtcQueryIntersector1(embreeGeo, "default");

	UPBP_ASSERT(embreeIntersector != nullptr);

	timer.Stop();
	const double treeConstructionTime = timer.GetLastElapsedTime();

	if (verbose)
	{
		std::cout
			<< std::setprecision(3)
			<< "   - BRE struct took  " << dataCopnversionTime + treeConstructionTime << " sec.\n"
			<< "       data conversion:   " << dataCopnversionTime << " sec.\n"
			<< "       tree construction: " << treeConstructionTime << " sec." << std::endl;
	}

	return numVerticesInMedium;
}

// ----------------------------------------------------------------------------------------------

/**
 * @brief	Destroy the data structure for BRE queries.
 */
void EmbreeBre::destroy( ) 
{
	if( embreePhotons != nullptr )
		delete[] embreePhotons;

	if( embreeGeo != nullptr )
		embree::rtcDeleteGeometry(embreeGeo);

	if( embreeIntersector != nullptr )
		rtcDeleteIntersector1 (embreeIntersector);

	embreePhotons = nullptr;
	numEmbreePhotons = 0;
	embreeGeo = nullptr;
	embreeIntersector = nullptr;
}

// ----------------------------------------------------------------------------------------------

/**
 * @brief	Destructor.
 */
EmbreeBre::~EmbreeBre()
{
	// The user should call destroy manually before deleting this object.
	UPBP_ASSERT( embreePhotons == nullptr );
	UPBP_ASSERT( numEmbreePhotons == 0 );
	UPBP_ASSERT( embreeGeo == nullptr );
	UPBP_ASSERT( embreeIntersector == nullptr );

	// Destroy anyway even if the user has forgotten to destroy().
	destroy();
}

// ----------------------------------------------------------------------------------------------

/**
 * @brief	Evaluates the beam radiance estimate for the given query ray.
 *
 * @param	beamType					   	Type of the beam.
 * @param	queryRay					   	The query ray (=beam) for the beam radiance estimate.
 * @param	segments					   	Full volume segments of media intersected by the ray.
 * @param	estimatorTechniques			   	the estimator techniques to use.
 * @param	raySamplingFlags			   	the ray sampling flags (\c
 * 											AbstractMedium::kOriginInMedium).
 * @param [in,out]	additionalRayDataForMis	(Optional) additional data needed for MIS weights
 * 											computations.
 *
 * @return	The accumulated radiance along the ray.
 */
Rgb EmbreeBre::evalBre(
	BeamType beamType,
	const Ray& queryRay,
	const VolumeSegments& segments,
	const uint estimatorTechniques,
	const uint raySamplingFlags,
	embree::AdditionalRayDataForMis* additionalRayDataForMis)
{
	UPBP_ASSERT(estimatorTechniques & PB2D);
	UPBP_ASSERT(raySamplingFlags == 0 || raySamplingFlags == AbstractMedium::kOriginInMedium);
	
	Rgb result(0);

	if(embreeIntersector == nullptr)
		return result;

	Rgb attenuation(1);
	float raySamplePdf = 1.0f;
	float raySampleRevPdf = 1.0f;

	/// Accumulate for each segment.
	for (VolumeSegments::const_iterator it = segments.begin(); it != segments.end(); ++it)
	{
		// Get segment medium.
		const AbstractMedium * medium = scene.mMedia[it->mMediumID];

		// Accumulate.
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
			embree::Ray embreeRay(toEmbreeV3f(queryRay.origin), toEmbreeV3f(queryRay.direction), it->mDistMin, it->mDistMax);
			embreeRay.setAdditionalData(medium, &segmentResult, beamType | estimatorTechniques, &queryRay, additionalRayDataForMis);
			embreeIntersector->intersect(embreeRay);
		}

		// Add to total result.
		result += attenuation * segmentResult;

		if (additionalRayDataForMis)
		{
			DebugImages & debugImages = *static_cast<DebugImages *>(additionalRayDataForMis->mDebugImages);
			debugImages.accumRgb2ToRgb(DebugImages::PB2D, attenuation);
			debugImages.ResetAccum2();
		}

		// Update attenuation.
		attenuation *= beamType == SHORT_BEAM ? it->mAttenuation / it->mRaySamplePdf :  // Short beams - no attenuation
			it->mAttenuation;
		if (!attenuation.isPositive())
			return result;

		// Update PDFs.
		raySamplePdf *= it->mRaySamplePdf;
		raySampleRevPdf *= it->mRaySampleRevPdf;
	}

	UPBP_ASSERT(!result.isNanInfNeg());

	return result;
}

/**
 * @brief	Evaluates the beam radiance estimate for the given query ray.
 *
 * @param	beamType					   	Type of the beam.
 * @param	queryRay					   	The query ray (=beam) for the beam radiance estimate.
 * @param	segments					   	Lite volume segments of media intersected by the ray.
 * @param	estimatorTechniques			   	the estimator techniques to use.
 * @param	raySamplingFlags			   	the ray sampling flags (\c
 * 											AbstractMedium::kOriginInMedium).
 * @param [in,out]	additionalRayDataForMis	(Optional) additional data needed for MIS weights
 * 											computations.
 *
 * @return	The accumulated radiance along the ray.
 */
Rgb EmbreeBre::evalBre(
	BeamType beamType,
	const Ray& queryRay,
	const LiteVolumeSegments& segments,
	const uint estimatorTechniques,
	const uint raySamplingFlags,
	embree::AdditionalRayDataForMis* additionalRayDataForMis)
{
	UPBP_ASSERT(beamType == LONG_BEAM);
	UPBP_ASSERT(estimatorTechniques & PB2D);
	UPBP_ASSERT(raySamplingFlags == 0 || raySamplingFlags == AbstractMedium::kOriginInMedium);
	
	Rgb result(0);

	if (embreeIntersector == nullptr)
		return result;

	Rgb attenuation(1);
	float raySamplePdf = 1.0f;
	float raySampleRevPdf = 1.0f;
	
	/// Accumulate for each segment.
	for (LiteVolumeSegments::const_iterator it = segments.begin(); it != segments.end(); ++it)
	{
		// Get segment medium.
		const AbstractMedium * medium = scene.mMedia[it->mMediumID];
		
		// Accumulate.
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
			embree::Ray embreeRay(toEmbreeV3f(queryRay.origin), toEmbreeV3f(queryRay.direction), it->mDistMin, it->mDistMax);			
			embreeRay.setAdditionalData(medium, &segmentResult, beamType | estimatorTechniques, &queryRay, additionalRayDataForMis);
			embreeIntersector->intersect(embreeRay);
		}
		
		// Add to total result.
		result += attenuation * segmentResult;

		if (additionalRayDataForMis)
		{
			DebugImages & debugImages = *static_cast<DebugImages *>(additionalRayDataForMis->mDebugImages);
			debugImages.accumRgb2ToRgb(DebugImages::PB2D, attenuation);
			debugImages.ResetAccum2();
		}

		// Update attenuation.
		attenuation *= medium->EvalAttenuation(queryRay, it->mDistMin, it->mDistMax);
		if (!attenuation.isPositive())
			return result;
		
		// Update PDFs.
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
 * @brief	Same as evalBre() except that no acceleration structure is used for the photon
 * 			lookups.
 *
 * @param	beamType	Type of the beam.
 * @param	queryRay	The query ray (=beam) for the beam radiance estimate.
 * @param	minT		Minimum value of the ray t parameter.
 * @param	maxT		Maximum value of the ray t parameter.
 * @param	medium  	Medium the current ray segment is in.
 *
 * @return	The accumulated radiance along the ray.
 */
Rgb EmbreeBre::evalBreBruteForce(
	BeamType beamType,
	const Ray &queryRay, 
	const float minT, 
	const float maxT, 
	const AbstractMedium* medium)
{
	Rgb result(0);

	// No photons - return zero.
	if(embreePhotons == nullptr)
		return result;
	Rgb attenuationCoeff, scatteringCoeff;

	// Make sure we're in a homogeneous medium.
	const int isHomogeneous = medium->IsHomogeneous();

	if( !isHomogeneous )
	{
		std::cerr << "Error: EmbreeBre::evalBreBruteForce() - only homogeneous media are currently supported" << std::endl;
		exit(2);
	}

	if( isHomogeneous )
	{
		attenuationCoeff = medium->GetAttenuationCoef( queryRay.target(minT) );
		scatteringCoeff  = medium->GetScatteringCoef( queryRay.target(minT) );
	}
		
	// Convert the query ray to embree format.
	embree::Ray embreeRay(toEmbreeV3f(queryRay.origin), toEmbreeV3f(queryRay.direction), minT, maxT);

	// Loop over all photons, test intersections and possibly accumulate radiance contributions.
	for( int i=0; i<numEmbreePhotons; i++ )
	{
		const EmbreePhoton& photon = embreePhotons[i];
		
		float photonIsectDist, photonIsectRadSqr;
		if( EmbreePhoton::testIntersectionBre(photonIsectDist, photonIsectRadSqr, queryRay, minT, maxT, photon.pos, photon.radiusSqr) )
		{
			result += 
				photon.flux *
				Rgb::exp( -photonIsectDist*attenuationCoeff ) * 
				PhaseFunction::Evaluate(queryRay.direction, photon.incDir, medium->MeanCosine() ) /
				(photon.radiusSqr);
		}
	}

	return result * scatteringCoeff * INV_PI_F;
}