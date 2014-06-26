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

#ifndef __PHGRID_HXX__
#define __PHGRID_HXX__

#include "Grid.hxx"

/**
 * @brief	Support for grid accelerated photon beams estimate.
 */
class AccelStruct : Grid<AccelStruct>
{
	/**
	 * @brief	Additional ray information.
	 */
	struct AdditionalRayData
	{
		const AbstractMedium * medium; //!< Medium the current ray segment is in.
		uint flags;                    //!< Ray flags (beam type and estimator techniques).
		Rgb accumResult;	           //!< Accumulated result.
		size_t invocation;             //!< Invocation number.

		const embree::AdditionalRayDataForMis* additionalDataForMis; //!< Data needed for MIS weights computation.
	};

public:

	/**
	 * @brief	Default constructor.
	 */
	AccelStruct():
		Grid(*this)
	{
	}

	/**
	 * @brief	Empty destructor.
	 */
	~AccelStruct()
	{
	}

	/**
	 * @brief	Builds the data structure for beam-beam queries.
	 * 			
	 * 			Constructs AABB of the beams and calls \c Grid::build() for them.
	 *
	 * @param	beams  	The beams.
	 * @param	verbose	Whether to print information about progress.
	 */
	void build(const PhotonBeamsArray & beams, int verbose)
	{
		// Handle bounding boxes.
		mAABB.setEmpty();
		for (PhotonBeamsArray::const_iterator it = beams.begin(); it != beams.end(); ++it)
		{
			mAABB = BoundingBox3(it->getAABB(), mAABB);
		}

		mPhotonBeams = &beams;
		Grid::build(mGridSize, verbose, mMaxBeamsInCell, mReductionType, mSeed);
		invocation = 0;
	}

	/**
	 * @brief	Evaluates the beam-beam estimate for the given query ray.
	 * 			
	 * 			Calls \c Grid::intersect() that in turn calls this \c AccelStruct::intersect() for
	 * 			each beam tested in cells.
	 *
	 * @param	queryRay				The query ray.
	 * @param	flags					Ray flags (beam type and estimator techniques).
	 * @param	medium					Medium the current ray segment is in.
	 * @param	mint					Minimum value of the ray t parameter.
	 * @param	maxt					Maximum value of the ray t parameter.
	 * @param [in,out]	gridStats   	Statistics to gather for the ray.
	 * @param	additionalDataForMis	(Optional) the additional data for MIS weights computation.
	 *
	 * @return	The estimate.
	 */
	Rgb evalBeamBeamEstimate(const Ray & queryRay, uint flags, const AbstractMedium * medium, float mint, float maxt, GridStats & gridStats, const embree::AdditionalRayDataForMis* additionalDataForMis = NULL)
	{
		AdditionalRayData data;
		data.accumResult = Rgb(0);
		data.flags = flags;
		data.medium = medium;
		data.invocation = ++invocation;
		data.additionalDataForMis = additionalDataForMis;
		Grid::intersect(queryRay, mint, maxt, (void *)(&data), gridStats);
		return data.accumResult;
	}

	/**
	 * @brief	Gets probability of selecting a beam (in case of beam reduction) in a grid cell
	 * 			containing the given position.
	 * 			
	 * 			Only calls \c Grid::pdf().
	 *
	 * @param	pos	The position.
	 *
	 * @return	The beam selection PDF.
	 */
	inline float getBeamSelectionPdf(const Pos & pos) const
	{
		return Grid::pdf(pos);
	}

	/**
	 * @brief	Compute the AABB of a beam (only used during tree construction).
	 *
	 * @param	beamptr	Pointer to the beam.
	 *
	 * @return	AABB of the beam.
	 */
	inline BoundingBox3 getAABB(const void *beamptr) const {
		return static_cast<const PhotonBeam *>(beamptr)->getAABB();
	}

	/**
	 * @brief	Compute the AABB of whole grid.
	 *
	 * @return	AABB of the whole grid.
	 */
	inline BoundingBox3 getAABB() const {
		return mAABB;
	}

	/**
	 * @brief	Compute the AABB of a beam segment (only used during tree construction).
	 *
	 * @param	beamptr 	Pointer to the beam.
	 * @param	splitMin	Beginning of the segment.
	 * @param	splitMax	End of the segment.
	 *
	 * @return	AABB of the segment.
	 */
	inline BoundingBox3 getSegmentAABB(const void *beamptr, const float splitMin, const float splitMax) const {
		return static_cast<const PhotonBeam *>(beamptr)->getSegmentAABB(splitMin, splitMax);
	}

	/**
	 * @brief	Return the total number of segments.
	 *
	 * @return	The total number of segments.
	 */
	inline uint size() const {
		return (uint)mPhotonBeams->size();
	}

	/**
	 * @brief	Intersects a single beam found in the grid.
	 *
	 * @param	beamptr	   	Pointer to the beam.
	 * @param	ray		   	The query ray.
	 * @param	mint	   	Original minimum value of the ray t parameter.
	 * @param	maxt	   	Original maximum value of the ray t parameter.
	 * @param	cellmint   	Minimum value of the ray t parameter inside the tested cell.
	 * @param	cellmaxt   	Maximum value of the ray t parameter inside the tested cell.
	 * @param	pdf		   	PDF of testing a beam in the tested cell.
	 * @param [in,out]	tmp	\c AccelStruct::AdditionalRayData.
	 *
	 * @return	always false, not used
	 */
	inline bool intersect(const void *beamptr, const Ray &ray,
		const float mint, const float maxt, const float cellmint, const float cellmaxt, const float pdf, void *tmp) const 
	{
		AdditionalRayData * rayData = static_cast<AdditionalRayData *>(tmp);
		const PhotonBeam & beam = *static_cast<const PhotonBeam *>(beamptr);
		
		beam.accumulate(ray, mint, maxt, cellmint, cellmaxt, pdf, rayData->accumResult, rayData->flags, rayData->medium, rayData->additionalDataForMis);
		
		return false;
	}

	/**
	 * @brief	Returns a pointer to a beam at the specified index.
	 *
	 * @param	index	Zero-based index of the beam.
	 *
	 * @return	The pointer to a beam at the specified index.
	 */
	inline const PhotonBeam * getObject(const uint index) const
	{
		return (&(*mPhotonBeams)[0]) + index;
	}

	/**
	 * @brief	Sets grid size.
	 *
	 * @param	aGridSize	Size of the grid.
	 */
	inline void setGridSize(uint aGridSize)
	{
		mGridSize = aGridSize;
	}

	/**
	 * @brief	Sets maximum number of tested beams in a single cell.
	 *
	 * @param	aMaxBeamsInCell	The maximum number of beams.
	 */
	inline void setMaxBeamsInCell(uint aMaxBeamsInCell)
	{
		mMaxBeamsInCell = aMaxBeamsInCell;
	}

	/**
	 * @brief	Sets type of reduction of numbers of tested beams in a single cell.
	 *
	 * @param	aReductionType	Type of the reduction.
	 */
	inline void setReductionType(uint aReductionType)
	{
		mReductionType = aReductionType;
	}

	/**
	 * @brief	Sets seed for sampling beams during the reduction.
	 *
	 * @param	aSeed	The seed.
	 */
	inline void setSeed(int aSeed)
	{
		mSeed = aSeed;
	}

private:
	const PhotonBeamsArray * mPhotonBeams;  //!< Holds all photon beams.
	
	BoundingBox3 mAABB; //!< Grid AABB.
	size_t invocation;  //!< Invocation number.
	uint mGridSize;     //!< Size of the grid.
	
	uint mMaxBeamsInCell;//!< The maximum number of tested beams in a single cell.
	uint mReductionType; //!< Type of the reduction of numbers of tested beams in cells.
	int mSeed;           //!< Seed for sampling beams during the reduction.
};

#endif // __PHGRID_HXX__