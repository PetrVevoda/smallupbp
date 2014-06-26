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

#ifndef __BEAMDENSITY_HXX__
#define __BEAMDENSITY_HXX__

#include "GridStats.hxx"
#include "..\Misc\Utils2.hxx"
#include "..\Misc\Framebuffer.hxx"

/**
 * @brief	Class that accumulates and outputs as false color images several statistics for
 * 			number of beams in grid cells.
 * 			
 * 			Used in \c VolLightTracer.hxx.
 */
class BeamDensity
{
public:

	/**
	 * @brief	Type of image to accumulate and output.
	 */
	enum ImgType
	{
		NONE = -1,    //!< No output.
		ABS = 0,	  //!< Absolute number of beams in all intersected cells.
		AVG = 1,	  //!< Average number of beams per intersected cell.
		CELLS = 2,    //!< Number of all intersected cells.
		OVERFULL = 3, //!< Number of intersected cells where beam reduction was carried out.
		ALL = 4       //!< Output all of the images above. Keep this item the last in this enumeration.
	};

	/**
	 * @brief	Default constructor.
	 */
	BeamDensity(){ }

	/**
	 * @brief	Prepares images of the given type and resolution.
	 * 			
	 * 			If maximum is specified, images are clamped and scaled according to it.
	 *
	 * @param	type	  	The type of the image.
	 * @param	resolution	The resolution of the image.
	 * @param	max		  	(Optional) the maximum value in the image.
	 */
	void Setup(const ImgType type, const Vec2f& resolution, const float max = -1)
	{
		mType = type;
		mResolution = resolution;
		mMax = max;

		mSize = int(resolution.get(0)) * int(resolution.get(1));
		
		mData.resize(ALL);
		mDataMax.resize(ALL, 0.0f);
		mContribMax.resize(ALL, 0.0f);		

		if (mType == ABS || mType == ALL)
			mData[ABS].resize(mSize, 0.0f);
		else
			mData[ABS].resize(0);

		if (mType == AVG || mType == ALL)
			mData[AVG].resize(mSize, 0.0f);
		else
			mData[AVG].resize(0);

		if (mType == CELLS || mType == ALL)
			mData[CELLS].resize(mSize, 0.0f);
		else
			mData[CELLS].resize(0);

		if (mType == OVERFULL || mType == ALL)
			mData[OVERFULL].resize(mSize, 0.0f);
		else
			mData[OVERFULL].resize(0);
	}

	/**
	 * @brief	Accumulates the given statistics to the image at the given pixel.
	 *
	 * @param	pixID	 	ID of the pixel.
	 * @param	gridStats	The grid statistics.
	 */
	void Accumulate(const int pixID, const GridStats& gridStats)
	{
		if (mType != NONE)
		{
			if (mType == ABS || mType == ALL)
			{
				mData[ABS][pixID] += (float)gridStats.intersectedBeams;
				mDataMax[ABS] = std::max(mDataMax[ABS], mData[ABS][pixID]);
				mContribMax[ABS] = std::max(mContribMax[ABS], (float)gridStats.intersectedBeams);
			}

			if ((mType == AVG || mType == ALL) && gridStats.intersectedCells)
			{
				float avg = (float)gridStats.intersectedBeams / (float)gridStats.intersectedCells;
				mData[AVG][pixID] += avg;
				mDataMax[AVG] = std::max(mDataMax[AVG], mData[AVG][pixID]);
				mContribMax[AVG] = std::max(mContribMax[AVG], avg);
			}

			if (mType == CELLS || mType == ALL)
			{
				mData[CELLS][pixID] += (float)gridStats.intersectedCells;
				mDataMax[CELLS] = std::max(mDataMax[CELLS], mData[CELLS][pixID]);
				mContribMax[CELLS] = std::max(mContribMax[CELLS], (float)gridStats.intersectedCells);
			}

			if (mType == OVERFULL || mType == ALL)
			{
				mData[OVERFULL][pixID] += (float)gridStats.overfullCells;
				mDataMax[OVERFULL] = std::max(mDataMax[OVERFULL], mData[OVERFULL][pixID]);
				mContribMax[OVERFULL] = std::max(mContribMax[OVERFULL], (float)gridStats.overfullCells);
			}
		}
	}

	/**
	 * @brief	Accumulates all data of the other \c BeamDensity object.
	 *
	 * @param	other	The other \c BeamDensity object.
	 */
	void Accumulate(const BeamDensity& other)
	{
		UPBP_ASSERT(mResolution.x == other.mResolution.x && mResolution.y == other.mResolution.y && mType == other.mType);

		if (mType != NONE)
		{
			for (int i = 0; i < ALL; i++)
			{
				if (mData[i].size())
				{
					mContribMax[i] = std::max(mContribMax[i], other.mContribMax[i]);
					
					for (int pixID = 0; pixID < mSize; pixID++)
					{
						mData[i][pixID] += other.mData[i][pixID];
						mDataMax[i] = std::max(mDataMax[i], mData[i][pixID]);
					}
				}
			}
		}
	}

	/**
	 * @brief	Outputs accumulated image(s) to a file.
	 * 			
	 * 			The name of the file is constructed as follows: \c A_beamdensB-dispmaxC-contmaxD.E
	 * 			where \c A = \c baseFileName, \c B = type of the image, \c C = maximum value in the
	 * 			image, \c D = maximum contribution to the image, \c E = extension.
	 *
	 * @param	baseFileName	The base filename.
	 * @param	extension   	The extension.
	 */
	void Output(const std::string& baseFileName, const std::string& extension) const
	{
		if (mType != NONE)
		{
			if (mType == ABS || mType == ALL)
				Output(baseFileName, extension, "ABS", ABS);

			if (mType == AVG || mType == ALL)
				Output(baseFileName, extension, "AVG", AVG);

			if (mType == CELLS || mType == ALL)
				Output(baseFileName, extension, "CELLS", CELLS);

			if (mType == OVERFULL || mType == ALL)
				Output(baseFileName, extension, "OVER", OVERFULL);
		}
	}

private:

	/**
	 * @brief	Normalizes image of the given type and saves it to a file.
	 * 			
	 * 			The name of the file is constructed as follows: \c A_beamdensB-dispmaxC-contmaxD.E
	 * 			where \c A = \c baseFileName, \c B = type of the image, \c C = maximum value in the
	 * 			image, \c D = maximum contribution to the image, \c E = extension.
	 *
	 * @param	baseFileName	The base filename.
	 * @param	extension   	The extension.
	 * @param	name			The name of the type of the image.
	 * @param	type			The type of the image.
	 */
	void Output(const std::string& baseFileName, const std::string& extension, const std::string& name, const ImgType type) const
	{
		UPBP_ASSERT(mData[type].size());

		Framebuffer framebuffer;
		framebuffer.Setup(mResolution);

		// Normalization either to the maximum given in setup or to actual maximum in data.
		const float max = mMax > 0 ? mMax : mDataMax[type];

		const int resX = int(mResolution.get(0));
		const int resY = int(mResolution.get(1));

		// Normalization to [0,1] range.
		for (int pixID = 0; pixID < mSize; pixID++)
		{
			const int x = pixID % resX;
			const int y = pixID / resX;

			if (max)
				framebuffer.AddColor(Vec2f(x, y), float2color(std::min(mData[type][pixID] / max, 1.0f)));
			else
				framebuffer.AddColor(Vec2f(x, y), float2color(0.0f));
		}

		// File name composition.
		std::ostringstream oss;
		oss << baseFileName << "_beamdens" << name << "-dispmax" << max << "-contmax" << mContribMax[type] << "." << extension;

		// Saving
		framebuffer.Save(oss.str());
	}

	Vec2f   mResolution;	//!< Image resolution.
	float   mMax;           //!< Target maximum value in the image.
	ImgType mType;          //!< Type of the image.
	int     mSize;          //!< Number of pixels of the image.

	std::vector<std::vector<float>>  mData;        //!< Accumulated data for the image.
	std::vector<float>	             mDataMax;     //!< Maximum value in the image.
	std::vector<float>	             mContribMax;  //!< Maximum contribution to the image.
};

#endif // __BEAMDENSITY_HXX__