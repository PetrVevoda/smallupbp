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

#ifndef __PHEMBREE_HXX__
#define __PHEMBREE_HXX__

#include "include\embree.h"
#include "common\ray.h"
#include "PhotonBeam.hxx"
#include "GridStats.hxx"

/**
 * @brief	Support for embree-based photon beams estimate.
 */
class AccelStruct
{
public:

	/**
	 * @brief	Default constructor.
	 */
	AccelStruct()
	{
		embreeBeamSegments = nullptr;
		numEmbreeBeamSegments = 0;
		embreeGeo = nullptr;
		embreeIntersector = nullptr;
	}

	/**
	 * @brief	Destructor.
	 * 			
	 * 			Destroys embree structures.
	 */
	~AccelStruct()
	{
		if (embreeBeamSegments != nullptr)
			delete[] embreeBeamSegments;

		if (embreeGeo != nullptr)
			embree::rtcDeleteGeometry(embreeGeo);

		if (embreeIntersector != nullptr)
			rtcDeleteIntersector1(embreeIntersector);

		embreeBeamSegments = nullptr;
		numEmbreeBeamSegments = 0;
		embreeGeo = nullptr;
		embreeIntersector = nullptr;
	}

	/**
	 * @brief	Builds the data structure for beam-beam queries.
	 *
	 * @param	beams  	The beams.
	 * @param	verbose	Whether to print information about progress.
	 */
	void build(const PhotonBeamsArray & beams, int verbose )
	{
		// Preferred length to width ratio for beam segments.
		const float LEN_WIDTH_RATIO = 10.0f;
		const int   MAX_BEAM_SUBSEGMENTS = 10;

		UPBP_ASSERT(embreeBeamSegments == nullptr);
		UPBP_ASSERT(numEmbreeBeamSegments == 0);
		UPBP_ASSERT(embreeGeo == nullptr);
		UPBP_ASSERT(embreeIntersector == nullptr);

		// Compute beam segments count.
		int numBeamSegments = 0;
		for (PhotonBeamsArray::const_iterator it = beams.begin(); it != beams.end(); ++it)
		{
			numBeamSegments +=
				determineBeamSegmentCount(it->mLength, (it->mStartRadius + it->mEndRadius) * 0.5f, LEN_WIDTH_RATIO, MAX_BEAM_SUBSEGMENTS);
		}
		
		if (verbose)
			std::cout << " + beam segment data struct construction over " << numBeamSegments << " beam segments." << std::endl;

		// Allocate embree beam segments.
		embreeBeamSegments = new EmbreeBeamSegment[numBeamSegments];
		numEmbreeBeamSegments = numBeamSegments;

		UPBP_ASSERT(embreeBeamSegments != nullptr);

		// Convert path vertices to embree photons.
		int beamSegmentIdx = 0;
		for (size_t i = 0; i < beams.size(); ++i)
		{
			const PhotonBeam * beam = &(beams[0]) + i;
			int numSegments = determineBeamSegmentCount(beam->mLength, (beam->mStartRadius + beam->mEndRadius) * 0.5f, LEN_WIDTH_RATIO, MAX_BEAM_SUBSEGMENTS);
			beamSegmentIdx +=
				chopUpBeam(embreeBeamSegments + beamSegmentIdx, beam, numSegments);
		}
		UPBP_ASSERT(beamSegmentIdx == numBeamSegments);

		// Build embree data structure
		embreeGeo = buildBeamSegmentTree(embreeBeamSegments, numBeamSegments);

		UPBP_ASSERT(embreeGeo != nullptr);

		// Retrieve the intersectable interface for this data structure.
		embreeIntersector = embree::rtcQueryIntersector1(embreeGeo, "default");

		UPBP_ASSERT(embreeIntersector != nullptr);
		invocation = 0;
	}
		
	/**
	 * @brief	Evaluates the beam-beam estimate for the given query ray.
	 *
	 * @param	queryRay				The query ray.
	 * @param	flags					Ray flags (beam type and estimator techniques).
	 * @param	medium					Medium the current ray segment is in.
	 * @param	mint					Minimum value of the ray t parameter.
	 * @param	maxt					Maximum value of the ray t parameter.
	 * @param [in,out]	gridStats   	Statistics to gather for the ray. Not used.
	 * @param	additionalDataForMis	(Optional) the additional data for MIS weights computation. Not used.
	 *
	 * @return	The estimate.
	 */
	Rgb evalBeamBeamEstimate(const Ray & queryRay, uint flags, const AbstractMedium * medium, float mint, float maxt, GridStats & gridStats, const embree::AdditionalRayDataForMis* additionalDataForMis = NULL)
	{
		Rgb result(0);
		embree::Ray embreeRay(toEmbreeV3f(queryRay.origin), toEmbreeV3f(queryRay.direction), mint, maxt);
		embreeRay.setAdditionalData(medium, &result, flags, &queryRay, additionalDataForMis);
		embreeRay.invocation = ++invocation;
		embreeIntersector->intersect(embreeRay);
		return result;
	}

	/**
	 * @brief	Has no sense here but needed in order to offer same interface as other structures.
	 *
	 * @param	pos	The position.
	 *
	 * @return	Constant 1.
	 */
	inline float getBeamSelectionPdf(const Pos & pos) const
	{
		return 1.0f;
	}

private:

	/**
	 * @brief	Holds one beam segment and handles embree based intersections with this segment.
	 */
	class EmbreeBeamSegment : public embree::Intersector1
	{
	public:
		const PhotonBeam * beamData; //!< Pointer to beam data.
		
		float minT;	//!< Beam segment start.
		float maxT;	//!< Beam segment end.

		/* 8 bytes total */

		/**
		 * @brief	Default constructor.
		 */
		EmbreeBeamSegment() {}

		/**
		 * @brief	Constructor.
		 *
		 * @param	aBeamData	Pointer to beam data.
		 * @param	aMinT	 	Beam segment start.
		 * @param	aMaxT	 	Beam segment end.
		 */
		EmbreeBeamSegment(const PhotonBeam * aBeamData, float aMinT, float aMaxT)
			: Intersector1(beamBeamIntersectFuncHomogeneous, beamBeamOccludedFunc)
		{
			set(aBeamData, aMinT, aMaxT);
		}

		/**
		 * @brief	Sets function pointers, pointer to beam data and start and end of the beam segment.
		 *
		 * @param	aBeamData	Pointer to beam data.
		 * @param	aMinT	 	Beam segment start.
		 * @param	aMaxT	 	Beam segment end.
		 */
		void set(const PhotonBeam * aBeamData, float aMinT, float aMaxT)
		{
			intersectPtr = beamBeamIntersectFuncHomogeneous;
			occludedPtr = beamBeamOccludedFunc;
			beamData = aBeamData;
			minT = aMinT;
			maxT = aMaxT;
		}

		/**
		 * @brief	BRE intersection function for a photon.
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
		static void beamBeamIntersectFuncHomogeneous(const embree::Intersector1* This, embree::Ray& ray)
		{
			const EmbreeBeamSegment* thisBeamSegment = (const EmbreeBeamSegment*)This;
			if (thisBeamSegment->beamData->mInvocation == ray.invocation)
				return;
			thisBeamSegment->beamData->mInvocation = ray.invocation;
			thisBeamSegment->beamData->accumulate(*ray.origRay, ray.tnear, ray.tfar, ray.tnear, ray.tfar, 1.0f, *static_cast<Rgb*>(ray.accumResult), ray.flags, ray.medium, ray.additionalRayDataForMis);
		}

		/**
		 * @brief	Beam-beam intersection function - should never be called.
		 *
		 * @param	This	   	This.
		 * @param [in,out]	ray	The ray.
		 *
		 * @return	Never returns.
		 */
		static bool beamBeamOccludedFunc(const embree::Intersector1* This, embree::Ray& ray)
		{
			std::cerr << "Error: beamBeamOccludedFunc() called - should not be called, makes not sense" << std::endl;
			exit(2);
		}
	};

	/**
	 * @brief	Builds a structure of 'virtual' objects - i.e. beam segments.
	 *
	 * @param [in,out]	beamSegments	The beam segments.
	 * @param	numBeamSegments			Number of beam segments.
	 *
	 * @return	The structure of beam segments.
	 */
	embree::RTCGeometry* buildBeamSegmentTree(EmbreeBeamSegment *beamSegments, int numBeamSegments)
	{
		embree::RTCGeometry* embreeGeo = embree::rtcNewVirtualGeometry(numBeamSegments, "default");

		for (int i = 0; i < numBeamSegments; ++i)
		{
			BoundingBox3 bbox = beamSegments[i].beamData->getSegmentAABB(beamSegments[i].minT, beamSegments[i].maxT);
			embree::rtcSetVirtualGeometryUserData(embreeGeo, i, i, 0);
			embree::rtcSetVirtualGeometryBounds(embreeGeo, i, &bbox.point1.x(), &bbox.point2.x());
			embree::rtcSetVirtualGeometryIntersector1(embreeGeo, i, &beamSegments[i]);
		}

		embree::rtcBuildAccel(embreeGeo, "default");
		embree::rtcCleanupGeometry(embreeGeo);
		return embreeGeo;
	}

	// ----------------------------------------------------------------------------------------------

	/**
	 * @brief	Determines the number of segments to chop a given beam into such that the ratio length /
	 * 			width is close to \c lenWidthRatio.
	 *
	 * @param	beamLength			  	Length of the beam.
	 * @param	beamRadius			  	The beam radius.
	 * @param	lenWidthRatio		  	The length width ratio.
	 * @param	maxBeamSubSegmentCount	Maximum number of beam sub segments.
	 *
	 * @return	The number of segments to chop a given beam into.
	 */
	inline int determineBeamSegmentCount(const float beamLength, const float beamRadius, const float lenWidthRatio, const int maxBeamSubSegmentCount)
	{
		return std::max(1, std::min(maxBeamSubSegmentCount, (int)(beamLength / (lenWidthRatio * beamRadius))));
	}

	/**
	 * @brief	Chops up beam.
	 *
	 * @param [in,out]	oEmbreeBeamSegments	Writes the resulting beam segments into this array.
	 * @param	photonBeam				   	PhotonBeam that corresponds to the beam that we want to
	 * 										chop up.
	 * @param	numSegments				   	Number of segments.
	 *
	 * @return	Number of segments into which this beam was chopped up.
	 */
	inline int chopUpBeam(
		EmbreeBeamSegment *oEmbreeBeamSegments,
		const PhotonBeam * photonBeam,
		int numSegments
		)
	{
		// Length of the sub-segments into which we are going to chop up the beam.
		const float segmentLength = photonBeam->mLength / numSegments;

		float offset = 0;
		for (int i = 0; i < numSegments; i++, offset += segmentLength)
		{
			oEmbreeBeamSegments[i].set(photonBeam, offset, offset + segmentLength);
		}

		return numSegments;
	}

	/**
	 * @brief	Converts the \c Pos to \c embree::Vector3f.
	 *
	 * @param	pos	The position.
	 *
	 * @return	Pos as an embree::Vector3f.
	 */
	INLINE embree::Vector3f toEmbreeV3f(const Pos& pos)
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
	INLINE embree::Vector3f toEmbreeV3f(const Dir& dir)
	{
		return embree::Vector3f(dir.x(), dir.y(), dir.z());
	}

	EmbreeBeamSegment *embreeBeamSegments;      //!< Our structure into which we've converted the input beams.
	int numEmbreeBeamSegments;                  //!< Number of elements in the embreeBeamSegments array.
	embree::RTCGeometry* embreeGeo;             //!< Embree's data structure storing the photon spheres.
	embree::RTCIntersector1* embreeIntersector; //!< Embree's intersector associated with the acceleration data structure.
	size_t invocation;                          //!< Invocation index.
};


#endif // __PHEMBREE_HXX__