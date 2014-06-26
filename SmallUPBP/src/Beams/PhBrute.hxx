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

#ifndef __PHBRUTE_HXX__
#define __PHBRUTE_HXX__

#include "PhotonBeam.hxx"
#include "GridStats.hxx"

/**
 * @brief	Support for brute force photon beams estimate.
 */
class AccelStruct
{
public:

	/**
	 * @brief	Default constructor.
	 */
	AccelStruct() 
	{
	}

	/**
	 * @brief	Empty destructor.
	 */
	~AccelStruct()
	{
	}

	/**
	 * @brief	Builds the data structure for beam-beam queries = just saves the reference to the given beams.
	 *
	 * @param	beams  	The beams.
	 * @param	verbose	Whether to print information about progress. Not used.
	 */
	void build(const PhotonBeamsArray & beams, int verbose)
	{
		mPhotonBeams = &beams;
	}

	/**
	 * @brief	Evaluates the beam-beam estimate for the given query ray.
	 * 			
	 * 			Calls \c PhotonBeam::accumulate() for each of the stored beams.
	 *
	 * @param	queryRay				The query ray.
	 * @param	flags					Ray flags (beam type and estimator techniques).
	 * @param	medium					Medium the current ray segment is in.
	 * @param	mint					Minimum value of the ray t parameter.
	 * @param	maxt					Maximum value of the ray t parameter.
	 * @param [in,out]	gridStats   	Statistics to gather for the ray. Not used.
	 * @param	additionalDataForMis	(Optional) the additional data for MIS weights computation.
	 *
	 * @return	The estimate.
	 */
	Rgb evalBeamBeamEstimate(const Ray & queryRay, uint flags, const AbstractMedium * medium, float mint, float maxt, GridStats & gridStats, const embree::AdditionalRayDataForMis* additionalDataForMis = NULL)
	{
		Rgb result(0);
		for (PhotonBeamsArray::const_iterator it = mPhotonBeams->begin(); it != mPhotonBeams->end(); ++it)
		{
			it->accumulate(queryRay, mint, maxt, mint, maxt, 1.0f, result, flags, medium, additionalDataForMis);
		}
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
	const PhotonBeamsArray * mPhotonBeams;  //!< Holds all photon beams.
};


#endif // __PHBRUTE_HXX__
