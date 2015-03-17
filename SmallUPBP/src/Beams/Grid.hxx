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

#ifndef __GRID_HXX__
#define __GRID_HXX__

#include <algorithm>

#include "PhotonBeam.hxx"
#include "GridStats.hxx"
#include "..\Path\StaticArray.hxx"

/**
 * @brief	A grid for storing photon beams.
 * 			
 * 			Regular grid with cube shaped cells. Its resolution in dimension of a maximum extent
 * 			of AABB of stored beams is set in \c Config.hxx. This gives size of the cells.
 * 			Resolution in other dimensions is then set to cover the AABB. A pointer to a beam is
 * 			stored in each cell the beam intersects.
 *
 * @typeparam	ObjectHandler	Type of the object handler.
 */
template<typename ObjectHandler>
class Grid
{
public:

	/**
	 * @brief	Constructor.
	 * 			
	 * 			Just saves reference to the objects to store.
	 *
	 * @param [in,out]	Objects	The objects to store.
	 */
	Grid(ObjectHandler & Objects) :
		mObjects(Objects)
	{
	}

	/**
	 * @brief	Finds a voxel containing the given local position in 1D.
	 *
	 * @param	pos 	Local position along one axis.
	 * @param	axis	Index of the axis.
	 *
	 * @return	An index of a voxel containing the given position.
	 */
	INLINE int posToVoxel(float pos, uint axis) const
	{
		return Utils::clamp<int>((int)pos, 0, mRes[axis] - 1);
	}

	/**
	 * @brief	Converts the given global position to a local one.
	 * 			
	 * 			Global position = position with respect to the word origin and in word units. Local
	 * 			position = position with respect to the AABB minimum point and with cell sizes as
	 * 			units.
	 *
	 * @param	pos	The global position.
	 *
	 * @return	Local position corresponding to the given global one.
	 */
	INLINE Pos toLocalPos(const Pos & pos) const
	{
		return *(Pos *)(&((pos - mAABB.point1) * mInvCellSize));
	}

	/**
	 * @brief	Gets an index of the given cell specified as an array of three integers.
	 *
	 * @param	p	The cell coordinates.
	 *
	 * @return	An index of the given cell.
	 */
	INLINE uint index(const int * p) const
	{
		return p[0] + mRes[0] * (p[1] + mRes[1] * p[2]);
	}

	/**
	 * @brief	Gets an index of the given cell specified as three integers.
	 *
	 * @param	x	The x cell coordinate.
	 * @param	y	The y cell coordinate.
	 * @param	z	The z cell coordinate.
	 *
	 * @return	An index of the given cell.
	 */
	INLINE uint index(int x, int y, int z) const
	{
		return x + mRes[0] * (y + mRes[1] * z);
	}

	/**
	 * @brief	An \c UpdateClass for \c buildloop() method that increments indices to \c mPointers.
	 */
	struct Increment
	{
		/**
		 * @brief	Increments index to \c mPointers of a pointer to the first beam stored in the given grid cell.
		 *
		 * @param [in,out]	grid	The grid.
		 * @param	cellindex   	Index of the cell.
		 * @param	beam			A beam (not used).
		 */
		static INLINE void update(Grid & grid, uint cellindex, const PhotonBeam * beam)
		{
			++grid.mCells[cellindex];
		}
	};

	/**
	 * @brief	An \c UpdateClass for \c buildloop() method that stores pointers to beams in cells.
	 */
	struct Store
	{
		/**
		 * @brief	Stores a pointer to the given beam for the given grid cell.
		 *
		 * @param [in,out]	grid	The grid.
		 * @param	cellindex   	Index of the cell.
		 * @param	beam			The beam.
		 */
		static INLINE void update(Grid & grid, uint cellindex, const PhotonBeam * beam)
		{
			grid.mPointers[--grid.mCells[cellindex]] = beam;
		}
	};

	/**
	 * @brief	Defines an alias representing the indices.
	 */
	typedef std::vector<uint> Indices;

	/**
	 * @brief	Performs a build loop over the beams that gathers pointers to them and their indices
	 * 			depending on the given \c UpdateClass.
	 *
	 * @typeparam	UpdateClass	Type of the update class. Either \c Increment or \c Store.
	 * @param [in,out]	testDuplicates	Array of indices of last updated beams in each cell used to
	 * 									avoid duplicities.
	 */
	template<typename UpdateClass>
	void buildloop(Indices & testDuplicates)
	{
		// For every beam
		for (uint i = 0; i < mObjects.size(); ++i)
		{
			// Get object AABB
			const PhotonBeam * beam = mObjects.getObject(i);
			const BoundingBox3 objaabb = beam->getAABB();
			
			const Dir extent = objaabb.point2 - objaabb.point1;
			const int maxAxis = beam->mRay.direction.abs().argMax();

			const int chopCount = (int)(extent[maxAxis] * mInvCellSize[maxAxis]) + 1;
			const float invChopCount = 1.0f / (float)chopCount;
			
			for (int chop = 0; chop < chopCount; ++chop)
			{
				BoundingBox3 aabb = beam->getSegmentAABB((chop)* invChopCount, (chop + 1) * invChopCount);
				
				const Pos start = toLocalPos(aabb.point1);
				const Pos end = toLocalPos(aabb.point2);

				const int istart[3] = { (int)(start.x()), (int)(start.y()), (int)(start.z()) };
				const int iend[3] = { (int)(end.x()) , (int)(end.y()), (int)(end.z())};
				
				uint id = index(istart);

				for (int z = istart[2]; z <= iend[2]; ++z, id += mIndexShift[2])
				for (int y = istart[1], idy = id; y <= iend[1]; ++y, idy += mIndexShift[1])
				for (int x = istart[0], idx = idy; x <= iend[0]; ++x, ++idx)
				{
					if (idx < 0) continue;
					if (idx >= testDuplicates.size()) break;
					
					if (testDuplicates[idx] != i)
					{
						testDuplicates[idx] = i;
						UpdateClass::update(*this, idx, beam);
					}
				}
			}
		}
	}

	/**
	 * @brief	Builds the grid for beams referenced in the constructor.
	 * 			
	 * 			Optionally it also reduces the number of beams in cells.
	 *
	 * @param	maxResolution 	The grid resolution in dimension of a maximum extent of beams AABB.
	 * @param	verbose		  	Whether to print information about progress.
	 * @param	maxBeamsInCell	(Optional) maximum number of tested beams in a single cell.
	 * @param	reductionType 	(Optional) type of the reduction of number of beams in cells.
	 * @param	seed		  	(Optional) seed for sampling beams during the reduction.
	 */
	void build(uint maxResolution, int verbose, uint maxBeamsInCell = 0, uint reductionType = 0, int seed = 1234)
	{
		mAABB = mObjects.getAABB();
		Dir extent = (mAABB.point2 - mAABB.point1);
		float size = extent.max() / maxResolution;

		mRes[0] = (uint)(extent.x() / size) + 1;
		mRes[1] = (uint)(extent.y() / size) + 1;
		mRes[2] = (uint)(extent.z() / size) + 1;

		if (verbose)
			std::cout << "Building grid with resolution: " << mRes[0] << "x" << mRes[1] << "x" << mRes[2] << std::endl
			<< "and AABB: " << mAABB.point1 << " - " << mAABB.point2 << std::endl;

		mIndexShift[0] = 1;
		mIndexShift[1] = mRes[0];
		mIndexShift[2] = mRes[0] * mRes[1];

		mCellSize = extent / Dir(mRes[0], mRes[1], mRes[2]);
		mCells.resize((mRes[0] + 1) * (mRes[1] + 1) * (mRes[2] + 1) + 1, 0);

		Indices testDuplicates(mCells.size(), (uint)mObjects.size());
		Indices test(mCells.size() + 1);
		
		mInvCellSize = Dir(1) / mCellSize;
		
		buildloop<Increment>(testDuplicates);
		
		/*if (verbose)
		{
			histogram();
		}*/
		
		uint accum = 0;
		for (Indices::iterator it = mCells.begin(), dup = testDuplicates.begin(), itcheck = test.begin(); it != mCells.end(); ++it, ++itcheck, ++dup)
		{
			UPBP_ASSERT(accum + *it >= accum);
			*itcheck = accum;
			accum += *it;
			*it = accum;
			*dup = (uint)mObjects.size();
		}
		if (verbose)
			std::cout << "Beam count " << mObjects.size() << ", Allocating indices: " << accum << std::endl;

		mPointers.resize(accum);
		
		buildloop<Store>(testDuplicates);
		
		// Check consistency
		for (Indices::iterator it = mCells.begin(), itcheck = test.begin(); it != mCells.end(); ++it, ++itcheck)
		{
			UPBP_ASSERT(*it == *itcheck);
		}

		reduceBeams(maxBeamsInCell, reductionType, seed);
	}

	/**
	 * @brief	Prints histogram of number of beams in cells.
	 */
	void histogram()
	{
		uint high = 0;
		for (Indices::iterator it = mCells.begin(); it != mCells.end(); ++it)
		{
			if (*it > high)
				high = *it;
		}

		const uint size = 100;
		Indices histogram(size, 0);
		uint total = 0;
		for (Indices::iterator it = mCells.begin(); it != mCells.end(); ++it)
		{
			++total;
			++histogram[(size - 1) * *it / (float)high];
		}

		float p = (float)high / size;
		for (Indices::iterator it = histogram.begin(); it != histogram.end(); ++it)
		{
			std::cout << " Cells with object count in [" << ((it - histogram.begin()) * p) << "," << ((it - histogram.begin() + 1) * p) << "]: " << *it << std::endl;
		}
	}

	/**
	 * @brief	Intersects the grid with the given ray
	 *
	 * @param	ray				 	The ray to intersect with.
	 * @param	mint			 	Minimum value of the ray t parameter. No intersections before it are considered.
	 * @param	maxt			 	Maximum value of the ray t parameter. No intersections after it are considered.
	 * @param [in,out]	tmp		 	\c AccelStruct::AdditionalRayData.
	 * @param [in,out]	gridStats	Statistics to gather for the ray.
	 */
	void intersect(const Ray & ray, float mint, float maxt, void * tmp, GridStats & gridStats)
	{
		const Dir invDir = 1.0f / ray.direction;
		float _mint, _maxt;
		
		if (mAABB.intersect(ray, invDir, _mint, _maxt))
		{
			if (_mint < mint) _mint = mint;
			if (_maxt > maxt) _maxt = maxt;

			if (likely(_maxt > _mint))
			{
				Dir delta = mCellSize * invDir;
				const Pos enter = toLocalPos(ray.target(_mint));
				int ipos[3] = { posToVoxel(enter.x(), 0), posToVoxel(enter.y(), 1), posToVoxel(enter.z(), 2) };
				uint cell = index(ipos);
				
				int shift[3], step[3];
				int check[3];
				
				for (int i = 0; i < 3; ++i)
				{
					if (ray.direction[i] >= 0.0f)
						shift[i] = 1, check[i] = mRes[i], step[i] = 1;
					else
						shift[i] = 0, check[i] = -1, step[i] = -1, delta[i] = -delta[i];
				}

				Dir l = (Pos(ipos[0] + shift[0], ipos[1] + shift[1], ipos[2] + shift[2]) - enter) * invDir * mCellSize;
				float t = _mint;
				
				do
				{
					// Intersect beams inside the current cell.

					const uint begin = mCells[cell];
					const uint end = mCells[cell + 1];
					const float _pdf = mPdfs[cell];

					int minAxis = l.argMin();

					float cell_mint = t;
					float cell_maxt = std::min(t + l[minAxis], _maxt);
					
					if (_pdf == 1.0f) // No reduction.
					{
						intersectAll(begin, end, ray, mint, maxt, cell_mint, cell_maxt, 1.0f, tmp);
						gridStats.intersectedBeams += end - begin;
					}
					else
					{
						if (mReductionType == PRESAMPLE)
							intersectPresampled(begin, end, ray, mint, maxt, cell_mint, cell_maxt, _pdf, tmp);
						else if (mReductionType == OFFSET)
							intersectOffsetted(begin, end, ray, mint, maxt, cell_mint, cell_maxt, _pdf, tmp);
						else if (mReductionType == RESAMPLE_FIXED)
							intersectFixedSampled(begin, end, ray, mint, maxt, cell_mint, cell_maxt, _pdf, tmp);
						else
							intersectSampled(begin, end, ray, mint, maxt, cell_mint, cell_maxt, _pdf, tmp);

						gridStats.intersectedBeams += mMaxBeamsInCell;
						gridStats.overfullCells++;
					}

					gridStats.intersectedCells++;

					// Move to the next cell.

					t += l[minAxis];
					l -= Dir(l[minAxis]);
					l[minAxis] = delta[minAxis];
					cell += mIndexShift[minAxis] * step[minAxis];
					ipos[minAxis] += step[minAxis];
					
					//UPBP_ASSERT(Grid::index(ipos) == cell);
					
					if (ipos[minAxis] == check[minAxis])
						return;
				} while (t < _maxt);
			}
		}
	}

	/**
	 * @brief	Gets probability of selecting a beam (in case of beam reduction) in a grid cell
	 * 			containing the given position.
	 *
	 * @param	pos	The position.
	 *
	 * @return	The beam selection PDF.
	 */
	float pdf(const Pos & pos) const
	{
		const Pos posl = toLocalPos(pos);
		int ipos[3] = { posToVoxel(posl.x(), 0), posToVoxel(posl.y(), 1), posToVoxel(posl.z(), 2) };
		uint cell = index(ipos);
		return mPdfs[cell];
	}

private:

	/**
	 * @brief	Intersects all beams inside a cell.
	 *
	 * @param	begin	   	Index of the first pointer to a beam of the tested cell in \c mPointers array.
	 * @param	end		   	Index of the last pointer to a beam of the tested cell in \c mPointers array.
	 * @param	ray		   	The ray.
	 * @param	mint	   	Original minimum value of the ray t parameter.
	 * @param	maxt	   	Original maximum value of the ray t parameter.
	 * @param	cellmint   	Minimum value of the ray t parameter inside the tested cell.
	 * @param	cellmaxt   	Maximum value of the ray t parameter inside the tested cell.
	 * @param	pdf		   	PDF of testing a beam in the tested cell.
	 * @param [in,out]	tmp	\c AccelStruct::AdditionalRayData.
	 */
	inline void intersectAll(uint begin, uint end, const Ray & ray, float mint, float maxt, const float cellmint, const float cellmaxt, float pdf, void * tmp)
	{
		for (uint index = begin; index != end; ++index)
		{
			mObjects.intersect(mPointers[index], ray, mint, maxt, cellmint, cellmaxt, pdf, tmp);
		}
	}

	/**
	 * @brief	Intersects all beams left inside a cell after presampling.
	 *
	 * @param	begin	   	Index of the first pointer to a beam of the tested cell in \c mPointers array.
	 * @param	end		   	Index of the last pointer to a beam of the tested cell in \c mPointers array.
	 * @param	ray		   	The ray.
	 * @param	mint	   	Original minimum value of the ray t parameter.
	 * @param	maxt	   	Original maximum value of the ray t parameter.
	 * @param	cellmint   	Minimum value of the ray t parameter inside the tested cell.
	 * @param	cellmaxt   	Maximum value of the ray t parameter inside the tested cell.
	 * @param	pdf		   	PDF of testing a beam in the tested cell.
	 * @param [in,out]	tmp	\c AccelStruct::AdditionalRayData.
	 */
	inline void intersectPresampled(uint begin, uint end, const Ray & ray, float mint, float maxt, const float cellmint, const float cellmaxt, float pdf, void * tmp)
	{
		const int firstSkipped = begin + mMaxBeamsInCell;
		uint index = begin;
		
		// Intersect the first mMaxBeamsInCell beams.
		for (; index != firstSkipped; ++index)
		{
			mObjects.intersect(mPointers[index], ray, mint, maxt, cellmint, cellmaxt, pdf, tmp);
		}
	}

	/**
	 * @brief	Intersects \c mMaxBeamsInCell beams inside a cell starting at a randomly chosen offset and continuing from the beginning if necessary.
	 *
	 * @param	begin	   	Index of the first pointer to a beam of the tested cell in \c mPointers array.
	 * @param	end		   	Index of the last pointer to a beam of the tested cell in \c mPointers array.
	 * @param	ray		   	The ray.
	 * @param	mint	   	Original minimum value of the ray t parameter.
	 * @param	maxt	   	Original maximum value of the ray t parameter.
	 * @param	cellmint   	Minimum value of the ray t parameter inside the tested cell.
	 * @param	cellmaxt   	Maximum value of the ray t parameter inside the tested cell.
	 * @param	pdf		   	PDF of testing a beam in the tested cell.
	 * @param [in,out]	tmp	\c AccelStruct::AdditionalRayData.
	 */
	inline void intersectOffsetted(uint begin, uint end, const Ray & ray, float mint, float maxt, const float cellmint, const float cellmaxt, float pdf, void * tmp)
	{
		uint n = end - begin;
		uint offset = begin + (uint)(mRng.GetFloat() * n);
		uint k = std::min(mMaxBeamsInCell, end - offset);

		// Intersect beams starting at the offset
		uint fromOffset = offset + k;
		for (uint index = offset; index < fromOffset; ++index)
		{
			mObjects.intersect(mPointers[index], ray, mint, maxt, cellmint, cellmaxt, pdf, tmp);
		}

		// and continue if necessary from the beginning.
		uint fromBegin = begin + mMaxBeamsInCell - k;
		for (uint index = begin; index < fromBegin; ++index)
		{
			mObjects.intersect(mPointers[index], ray, mint, maxt, cellmint, cellmaxt, pdf, tmp);
		}
	}

	/**
	 * @brief	Intersects \c mMaxBeamsInCell randomly chosen beams inside a cell.
	 *
	 * @param	begin	   	Index of the first pointer to a beam of the tested cell in \c mPointers array.
	 * @param	end		   	Index of the last pointer to a beam of the tested cell in \c mPointers array.
	 * @param	ray		   	The ray.
	 * @param	mint	   	Original minimum value of the ray t parameter.
	 * @param	maxt	   	Original maximum value of the ray t parameter.
	 * @param	cellmint   	Minimum value of the ray t parameter inside the tested cell.
	 * @param	cellmaxt   	Maximum value of the ray t parameter inside the tested cell.
	 * @param	pdf		   	PDF of testing a beam in the tested cell.
	 * @param [in,out]	tmp	\c AccelStruct::AdditionalRayData.
	 */
	inline void intersectFixedSampled(uint begin, uint end, const Ray & ray, float mint, float maxt, const float cellmint, const float cellmaxt, float pdf, void * tmp)
	{
		uint n = end - begin;

		// Intersect mMaxBeamsInCell randomly chosen beams.
		for (uint i = 0; i < mMaxBeamsInCell; ++i)
		{
			float r = mRng.GetFloat();
			while (r == 1.0f) r = mRng.GetFloat();
			uint index = begin + (uint)(r * n);
			UPBP_ASSERT(index < end);

			mObjects.intersect(mPointers[index], ray, mint, maxt, cellmint, cellmaxt, pdf, tmp);
		}
	}

	/**
	 * @brief	For each beam decides with the given probability whether to intersect it or skip.
	 *
	 * @param	begin	   	Index of the first pointer to a beam of the tested cell in \c mPointers array.
	 * @param	end		   	Index of the last pointer to a beam of the tested cell in \c mPointers array.
	 * @param	ray		   	The ray.
	 * @param	mint	   	Original minimum value of the ray t parameter.
	 * @param	maxt	   	Original maximum value of the ray t parameter.
	 * @param	cellmint   	Minimum value of the ray t parameter inside the tested cell.
	 * @param	cellmaxt   	Maximum value of the ray t parameter inside the tested cell.
	 * @param	pdf		   	PDF of testing a beam in the tested cell.
	 * @param [in,out]	tmp	\c AccelStruct::AdditionalRayData.
	 */
	inline void intersectSampled(uint begin, uint end, const Ray & ray, float mint, float maxt, const float cellmint, const float cellmaxt, float pdf, void * tmp)
	{
		// For each beam decide with probability PDF whether to intersect it or skip.
		for (uint index = begin; index != end; ++index)
		{
			if (mRng.GetFloat() < pdf)
				mObjects.intersect(mPointers[index], ray, mint, maxt, cellmint, cellmaxt, pdf, tmp);
		}
	}	

	/**
	 * @brief	Reduce number of beams in cells.
	 *
	 * @param	maxBeamsInCell	The maximum number of tested beams in a single cell.
	 * @param	reductionType 	Type of the reduction.s
	 * @param	seed		  	Seed for sampling beams.
	 */
	void reduceBeams(uint maxBeamsInCell, uint reductionType, int seed)
	{
		mMaxBeamsInCell = maxBeamsInCell;
		mReductionType = static_cast<BeamReduction>(reductionType);
		mRng = Rng(seed);
		
		size_t cells = mCells.size() - 1;
		mPdfs.resize(cells);
		
		if (mMaxBeamsInCell == 0) // no reduction
		{
			for (size_t i = 0; i < cells; i++) mPdfs[i] = 1.0f;
		}
		else  if (mReductionType == PRESAMPLE)
		{
			for (size_t i = 0; i < cells; i++)
			{
				const uint begin = mCells[i];
				const uint end = mCells[i + 1];
				const uint beams = end - begin;

				if (beams <= mMaxBeamsInCell) mPdfs[i] = 1.0f;
				else
				{
					mPdfs[i] = (float)mMaxBeamsInCell / (float)beams;

					// Random shuffle first mMaxBeamsInCell beams.
					for (uint i = 0; i < mMaxBeamsInCell; ++i)
					{
						float r = mRng.GetFloat();
						while (r == 1.0f) r = mRng.GetFloat();
						uint j = i + (uint)(r * (beams - i));
						UPBP_ASSERT(j < beams);
						std::swap(mPointers[begin + i], mPointers[begin + j]);
					}
				}
			}
		}
		else if (mReductionType == OFFSET)
		{
			for (size_t i = 0; i < cells; i++)
			{
				const uint begin = mCells[i];
				const uint end = mCells[i + 1];
				const uint beams = end - begin;

				if (beams <= mMaxBeamsInCell) mPdfs[i] = 1.0f;
				else
				{
					mPdfs[i] = (float)mMaxBeamsInCell / (float)beams;

					// Random shuffle all beams.
					for (uint i = beams - 1; i > 0; --i)
					{
						float r = mRng.GetFloat();
						while (r == 1.0f) r = mRng.GetFloat();
						uint j = (uint)(r * (i + 1));
						UPBP_ASSERT(j <= i);
						std::swap(mPointers[begin + i], mPointers[begin + j]);
					}
				}
			}
		}
		else
		{
			for (size_t i = 0; i < cells; i++)
			{
				const uint beams = mCells[i + 1] - mCells[i];

				if (beams <= mMaxBeamsInCell) mPdfs[i] = 1.0f;
				else mPdfs[i] = (float)mMaxBeamsInCell / (float)beams;
			}
		}
	}

	/**
	 * @brief	Defines an alias representing the pointers to beams.
	 */
	typedef std::vector<const PhotonBeam *> Pointers;

	/**
	 * @brief	Defines an alias representing the PDFs.
	 */
	typedef std::vector<float> Pdfs;

	ObjectHandler & mObjects;     //!< The beams.
	Indices mCells;               //!< For each cell contains index of a pointer to its first beam in \c mPointers array.
	Pointers mPointers;           //!< The pointers to beams.
	Pdfs mPdfs;                   //!< The PDFs of intersecting beams in cells.
	uint mRes[3];                 //!< Grid resolution.
	uint mIndexShift[3];	      //!< The index shift.
	uint mMaxBeamsInCell;         //!< The maximum number of tested beams in a single cell.
	BeamReduction mReductionType; //!< Type of the reduction of numbers of tested beams in cells.
	Dir mCellSize;                //!< Size of a cell.
	Dir mInvCellSize;             //!< Inverse of the size of a cell.
	BoundingBox3 mAABB;           //!< Axis aligned bounding box of the beams.
	Rng mRng;                     //!< Random number generator for sampling beams during reduction.
};
#endif