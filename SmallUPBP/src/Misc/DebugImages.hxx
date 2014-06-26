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

#ifndef __DEBUGIMAGES_HXX__
#define __DEBUGIMAGES_HXX__

#include <string>
#include <sstream>

#include "Framebuffer.hxx"

/**
 * @brief	Class that controls output of debugging images.
 * 			
 * 			Debugging images show contribution related to a technique and/or camera vs light subpath length. Also MIS weights of used techniques can be output.
 */
class DebugImages
{
public:

	/**
	 * @brief	Type of debugging images to generate.
	 */
	enum DebugOptions
	{
		NONE,                 //!< No debugging images.
		SIMPLE_PYRAMID,       //!< Contribution related to a camera vs light subpath length.
		PER_TECHNIQUE,        //!< Contribution related to a technique.
		PYRAMID_PER_TECHNIQUE //!< Contribution related to a camera vs light subpath length and a technique.
	};

	/**
	 * @brief	Techniques used for generating debugging images.
	 */
	enum Techniques
	{
		BPT,
		SURFACE_PHOTON_MAPPING,
		PP3D,
		PB2D,
		BB1D,
		TECHNIQUE_COUNT
	};

	/**
	 * @brief	Whether to output MIS weights of used techniques.
	 */
	enum WeightsOptions
	{
		DO_NOT_OUTPUT_WEIGHTS,
		OUTPUT_WEIGHTS
	};

	/**
	 * @brief	How to deal with MIS weights in images.
	 */
	enum MisWeights
	{
		IGNORE_WEIGHTS,
		MULTIPLY_BY_WEIGHTS,
		OUTPUT_BOTH_VERSIONS
	};

	/**
	 * @brief	Default constructor.
	 */
	DebugImages() :mAccumulation(0)
	{
		mTechniqueNames.resize(TECHNIQUE_COUNT);
		mTechniqueNames[BPT] = "BPT";
		mTechniqueNames[SURFACE_PHOTON_MAPPING] = "SPM";
		mTechniqueNames[PP3D] = "PP3D";
		mTechniqueNames[PB2D] = "PB2D";
		mTechniqueNames[BB1D] = "BB1D";
	}

	/**
	 * @brief	Setups the images according to the given \c DebugImages.
	 *
	 * @param	aDebugImages	The debug images.
	 */
	void Setup(const DebugImages & aDebugImages)
	{
		Setup(aDebugImages.mMaxPathLength, aDebugImages.mResolution, aDebugImages.mDebugOptions, aDebugImages.mWeightsOptions, aDebugImages.mMisWeights);
	}

	/**
	 * @brief	Setups the images according to the given parameters.
	 *
	 * @param	aMaxPathLength 	The maximum path length (for pyramid images).
	 * @param	aResolution	   	The resolution of images.
	 * @param	aDebugOptions  	Type of images to generate.
	 * @param	aWeightsOptions	Whether to output MIS weights of used techniques.
	 * @param	aMisWeights	   	How to deal with MIS weights in images.
	 */
	void Setup(int aMaxPathLength, Vec2f aResolution, DebugOptions aDebugOptions, WeightsOptions aWeightsOptions, MisWeights aMisWeights)
	{
		mDebugOptions = aDebugOptions;
		mMaxPathLength = aMaxPathLength;
		mResolution = aResolution;
		mWeightsOptions = aWeightsOptions;
		mMisWeights = aMisWeights;
		
		int multFactor = mMisWeights == OUTPUT_BOTH_VERSIONS ? 2 : 1;
		int accumSize = mMaxPathLength * TECHNIQUE_COUNT * multFactor;
		
		mRGB.resize(accumSize);
		mRGB2.resize(accumSize);
		mRGBWeighted.resize(accumSize);
		mRGB2Weighted.resize(accumSize);
		mWeights.resize(accumSize);
		
		mCompletelyIgnore = mDebugOptions == NONE && aWeightsOptions == DO_NOT_OUTPUT_WEIGHTS;
		if (mCompletelyIgnore)
			return;
		
		int pyramidSize = (mMaxPathLength * mMaxPathLength + mMaxPathLength * 3) / 2; // There is 1/2(L^2 + 3L) images in pyramid
		if (mDebugOptions == NONE)
		{
			mDebugImagesPerTechnique = mTechniquesInDebugImages = 0;
		}
		else
		{
			mTechniquesInDebugImages = mDebugOptions == SIMPLE_PYRAMID ? 1 : TECHNIQUE_COUNT;
			mDebugImagesPerTechnique = mDebugOptions == PER_TECHNIQUE ? 1 : pyramidSize;
		}
		
		mWeightImagesStart = mDebugImagesPerTechnique * mTechniquesInDebugImages * multFactor;
		mWeightImagesCount = mWeightsOptions == DO_NOT_OUTPUT_WEIGHTS ? 0 : TECHNIQUE_COUNT;
		
		frameBuffers.resize(mWeightImagesStart + mWeightImagesCount);
		for (FrameBuffers::iterator it = frameBuffers.begin(); it != frameBuffers.end(); ++it)
		{
			it->Setup(aResolution);
		}
	}

	/**
	 * @brief	Adds a sample to images.
	 *
	 * @param	aCameraPathLength	Length of the camera subpath.
	 * @param	aLightPathLength 	Length of the light subpath.
	 * @param	aTechnique		 	Used technique.
	 * @param	aSample			 	Position of the sample.
	 * @param	aColor			 	Color of the sample.
	 * @param	aWeightedColor   	Weighted color of the sample.
	 * @param	aMisWeight		 	MIS weight of the sample.
	 */
	void addSample(int aCameraPathLength, int aLightPathLength, Techniques aTechnique, const Vec2f& aSample, Rgb aColor, const Rgb& aWeightedColor, float aMisWeight)
	{
		if (mCompletelyIgnore)
			return;
		
		int pathLength = aCameraPathLength + aLightPathLength;
		int pyramidIndex = (pathLength * pathLength + pathLength) / 2 - 1 + aLightPathLength;
		if (aTechnique == BPT)
			aColor /= pathLength + 1;
		else
			aColor /= pathLength - 1;
		
		switch (mDebugOptions)
		{
		case NONE:break;
		case SIMPLE_PYRAMID:
			__addSample(pyramidIndex, aSample, aColor, aWeightedColor);
			break;
		case PER_TECHNIQUE:
			__addSample((uint)(aTechnique), aSample, aColor, aWeightedColor);
			break;
		case PYRAMID_PER_TECHNIQUE:
			__addSample(((uint)aTechnique) * mDebugImagesPerTechnique + pyramidIndex, aSample, aColor, aWeightedColor);
			break;
		default:
			UPBP_ASSERT(false);
		}
		
		switch (mWeightsOptions)
		{
		case DO_NOT_OUTPUT_WEIGHTS:
			break;
		case OUTPUT_WEIGHTS:
			frameBuffers[((int)aTechnique) + mWeightImagesStart].AddColor(aSample, Rgb(aMisWeight));
			break;
		default:
			UPBP_ASSERT(false);
		}
	}

	/**
	 * @brief	Adds an already accumulated light sample.
	 * 			
	 * 			Expects the one or more samples with the same position and camera subpath length are
	 * 			already accumulated and only need to be multiplied by the given factor.
	 *
	 * @param	aCameraPathLength	Length of the camera subpath.
	 * @param	aTechnique		 	Used technique.
	 * @param	aSample			 	Position of the sample.
	 * @param	aMult			 	The multiplication factor.
	 */
	void addAccumulatedLightSample(int aCameraPathLength, Techniques aTechnique, const Vec2f& aSample, const Rgb& aMult)
	{
		if (mCompletelyIgnore)
			return;
		
		int maxLightPath = mMaxPathLength - aCameraPathLength;
		for (int lightPathLength = 1; lightPathLength <= maxLightPath; ++lightPathLength)
		{
			const int index = mMaxPathLength * (int)aTechnique +lightPathLength - 1;
			addSample(aCameraPathLength, lightPathLength, aTechnique, aSample, aMult * mRGB[index], aMult * mRGBWeighted[index], mWeights[index]);
		}
	}

	/**
	 * @brief	Accumulates data from the given \c DebugImages.
	 *
	 * @param	aDebugImages	Images to accumulate.
	 * @param	aIterations 	Number of used iterations.
	 */
	void Accumulate(const DebugImages & aDebugImages, int aIterations)
	{
		if (mCompletelyIgnore)
			return;
		
		UPBP_ASSERT(aDebugImages.frameBuffers.size() == frameBuffers.size());
		if (aIterations == 0)
			return;
		
		FrameBuffers::const_iterator srcIt = aDebugImages.frameBuffers.begin();
		float scaling = 1.0f / aIterations;
		for (FrameBuffers::iterator dstIt = frameBuffers.begin(); dstIt != frameBuffers.end(); ++dstIt, ++srcIt)
		{
			dstIt->AddScaled(*srcIt, scaling);
		}
		
		++mAccumulation;
	}

	/**
	 * @brief	Outputs the images.
	 *
	 * @param	aPrefix   	Prefix of file names.
	 * @param	aExtension	Extension of file names.
	 */
	void Output(const std::string & aPrefix, const std::string & aExtension)
	{
		if (mCompletelyIgnore)
			return;
		
		// First scale all the images by accumulation factor.
		float scaling = mAccumulation == 0 ? 1.0f : 1.0f / mAccumulation;
		for (FrameBuffers::iterator it = frameBuffers.begin(); it != frameBuffers.end(); ++it)
		{
			it->Scale(scaling);
		}
		
		// Output images according to options.
		switch (mDebugOptions)
		{
		case NONE:break;
		case SIMPLE_PYRAMID:
			for (int pathLength = 1; pathLength <= mMaxPathLength; ++pathLength)
			{
				std::ostringstream os;
				os << aPrefix << "_L" << pathLength << "_";
				for (int s = 0; s <= pathLength; ++s)
				{

					int pyramidIndex = (pathLength * pathLength + pathLength) / 2 - 1 + s;
					std::ostringstream os2;
					os2 << os.str() << "S" << s << "_T" << (pathLength - s);
					__outputImage(pyramidIndex, os2.str(), aExtension);
				}
			}
			break;
		case PER_TECHNIQUE:
			for (int technique = 0; technique < TECHNIQUE_COUNT; ++technique)
			{
				__outputImage(technique, aPrefix + "_" + mTechniqueNames[technique], aExtension);
			}
			break;
		case PYRAMID_PER_TECHNIQUE:
			for (int pathLength = 1; pathLength <= mMaxPathLength; ++pathLength)
			{
				std::ostringstream os;
				os << aPrefix << "_L" << pathLength << "_";
				for (int s = 0; s <= pathLength; ++s)
				{

					const int pyramidIndex = (pathLength * pathLength + pathLength) / 2 - 1 + s;
					std::ostringstream os2;
					os2 << os.str() << "S" << s << "_T" << (pathLength - s);
					for (int technique = 0; technique < TECHNIQUE_COUNT; ++technique)
					{
						const int index = mDebugImagesPerTechnique * technique + pyramidIndex;
						__outputImage(index, os2.str() + "_" + mTechniqueNames[technique], aExtension);
					}
				}
			}
			break;
		default:
			UPBP_ASSERT(false);
		}
		switch (mWeightsOptions)
		{
		case DO_NOT_OUTPUT_WEIGHTS:
			break;
		case OUTPUT_WEIGHTS:
			for (int technique = 0; technique < TECHNIQUE_COUNT; ++technique)
			{
				frameBuffers[mWeightImagesStart + technique].Save((aPrefix + "_WEIGHT_" + mTechniqueNames[technique] + "." + aExtension));
			}
			break;
		default:
			UPBP_ASSERT(false);
		}
	}

	// The following functions must be const - so they change only mutable variables

	/**
	 * @brief	Resets accumulated RGB and MIS weights.
	 */
	void ResetAccum() const
	{
		if (mCompletelyIgnore)
			return;

		for (size_t i = 0; i < mRGB.size(); ++i)
		{
			mWeights[i] = 0.0f;
			mRGB2[i] = mRGB[i] = Rgb(0.0f);
			mRGBWeighted[i] = mRGB2Weighted[i] = Rgb(0.0f);
		}
	}

	/**
	 * @brief	Resets accumulated RGB.
	 */
	void ResetAccum2() const
	{
		if (mCompletelyIgnore)
			return;

		for (size_t i = 0; i < mRGB.size(); ++i)
		{
			mRGB2[i] = mRGB2Weighted[i] = Rgb(0.0f);
		}
	}

	/**
	 * @brief	Accumulates RGB value and MIS weight of the given light path length and technique
	 * 			(e.g used during PB2D).
	 *
	 * @param	aLightPathLength	Length of the light subpath.
	 * @param	aTechnique			Used technique.
	 * @param	aRGB				The RGB.
	 * @param	aMisWeight			The MIS weight.
	 */
	INLINE void accumRgbWeight(const int aLightPathLength, const Techniques aTechnique, const Rgb & aRGB, const float aMisWeight) const
	{
		const int index = mMaxPathLength * (int)aTechnique + aLightPathLength - 1;
		mRGBWeighted[index] += aRGB * aMisWeight;
		mRGB[index] += aRGB;
		mWeights[index] += aMisWeight;
	}

	/**
	 * @brief	Accumulates RGB value to the second temporary stack and MIS weight of the given light
	 * 			path length and technique  (e.g used during PB2D).
	 *
	 * @param	aLightPathLength	Length of the light subpath.
	 * @param	aTechnique			Used technique.
	 * @param	aRGB				The RGB.
	 * @param	aMisWeight			The MIS weight.
	 */
	INLINE void accumRgb2Weight(const int aLightPathLength, const Techniques aTechnique, const Rgb & aRGB, const float aMisWeight) const
	{
		const int index = mMaxPathLength * (int)aTechnique + aLightPathLength - 1;
		mRGB2Weighted[index] += aRGB * aMisWeight;
		mRGB2[index] += aRGB;
		mWeights[index] += aMisWeight;
	}

	/**
	 * @brief	Accumulates RGB value from the second temporary stack to the first.
	 *
	 * @param	aTechnique	Used technique.
	 * @param	aMult	  	Multiplication factor.
	 */
	INLINE void accumRgb2ToRgb(const Techniques aTechnique, const Rgb & aMult) const
	{
		if (mCompletelyIgnore)
			return;
		
		const int index = mMaxPathLength * (int)aTechnique - 1;
		for (size_t l = 1; l <= mMaxPathLength; ++l)
		{
			mRGB[index + l] += aMult * mRGB2[index + l];
			mRGBWeighted[index + l] += aMult * mRGB2Weighted[index + l];
		}
	}

	/**
	 * @brief	Sets temporary RGB and MIS weight  (e.g used during direct illumination).
	 *
	 * @param	aRGB	  	The RGB.
	 * @param	aMisWeight	The MIS weight.
	 */
	INLINE void setTempRgbWeight(const Rgb & aRGB, const float aMisWeight) const
	{
		mTempRGB = aRGB;
		mTempWeight = aMisWeight;
	}

	/**
	 * @brief	Returns temporary radiance (e.g used during direct illumination).
	 *
	 * @return	The temporary RGB.
	 */
	INLINE const Rgb & getTempRGB() const
	{
		return mTempRGB;
	}

	/**
	 * @brief	Returns temporary MIS weight (e.g used during direct illumination).
	 *
	 * @return	The temporary MIS weight.
	 */
	INLINE const float & getTempMisWeight() const
	{
		return mTempWeight;
	}

	/**
	 * @brief	Resets temporarily stored MIS weight and RGB.
	 */
	INLINE void ResetTemp() const
	{
		mTempRGB = Rgb(0.0f);
		mTempWeight = 0.0f;
	}
private:

	/**
	 * @brief	Adds a sample to image in the pyramid at the specified index.
	 *
	 * @param	aIndex		  	Index of the image to add to.
	 * @param	aSample		  	Position of the sample.
	 * @param	aColor		  	Color of the sample.
	 * @param	aWeightedColor	Weighted color of the sample.
	 */
	INLINE void __addSample(uint aIndex, const Vec2f& aSample, const Rgb& aColor, const Rgb& aWeightedColor)
	{
		switch (mMisWeights)
		{
		case IGNORE_WEIGHTS:
			frameBuffers[aIndex].AddColor(aSample, aColor);
			break;
		case MULTIPLY_BY_WEIGHTS:
			frameBuffers[aIndex].AddColor(aSample, aWeightedColor);
			break;
		case OUTPUT_BOTH_VERSIONS:
			frameBuffers[aIndex * 2].AddColor(aSample, aColor);
			frameBuffers[aIndex * 2 + 1].AddColor(aSample, aWeightedColor);
			break;
		}
	}

	/**
	 * @brief	Outputs image at the specified index to a file with the given prefix and extension.
	 *
	 * @param	aIndex	Index of the image to output.
	 * @param	prefix	Prefix of file name.
	 * @param	ext   	Extension of file name.
	 */
	INLINE void __outputImage(int aIndex, const std::string & prefix, const std::string & ext)
	{
		switch (mMisWeights)
		{
		case IGNORE_WEIGHTS:
			frameBuffers[aIndex].Save((prefix + "_UNWEIGHTED." + ext));
			break;
		case MULTIPLY_BY_WEIGHTS:
			frameBuffers[aIndex].Save((prefix + "_WEIGHTED." + ext));
			break;
		case OUTPUT_BOTH_VERSIONS:
			frameBuffers[aIndex * 2].Save((prefix + "_UNWEIGHTED." + ext));
			frameBuffers[aIndex * 2 + 1].Save((prefix + "_WEIGHTED." + ext));
			break;
		}
	}

	typedef std::vector<std::string> Strings;
	typedef std::vector<Framebuffer> FrameBuffers;
	typedef std::vector<Rgb> Rgbs;
	typedef std::vector<float> Floats;
	
	FrameBuffers frameBuffers;  //!< Frame buffers of the images.
	Strings mTechniqueNames;	//!< List of names of techniques.
	
	mutable Rgbs mRGB, mRGB2, mRGBWeighted, mRGB2Weighted; //!< RGB values.
	mutable Floats mWeights, mWeights2;                    //!< MIS weights.
	mutable Rgb mTempRGB;                                  //!< Temporarily stored RGB values.
	mutable float mTempWeight;                             //!< Temporarily stored MIS weights.
	
	int mMaxPathLength; //!< Maximum path length (for pyramid images).
	Vec2f mResolution;  //!< Resolution of the images.
	
	int mDebugImagesPerTechnique;   //!< Number of images to generate per technique (1 or the size of the pyramid).
	int mTechniquesInDebugImages;   //!< Number of techniques to generate images for (1 or the number of techniques).
	
	int mWeightImagesStart; //!< Index of the first image of weights in the array of framebuffers.
	int mWeightImagesCount; //!< Number of images of weights (zero or the number of techniques).
	
	int mAccumulation;  //!< Number of accumulations taken.
	
	bool mCompletelyIgnore; //!< If true, no image will be generated and output.

	DebugOptions mDebugOptions;     //!< Type of debugging images to generate.
	WeightsOptions mWeightsOptions; //!< Whether to output MIS weights of used techniques.
	MisWeights mMisWeights;         //!< How to deal with MIS weights in images.
};

#endif //__DEBUGIMAGES_HXX__