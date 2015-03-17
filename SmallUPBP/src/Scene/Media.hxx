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

#ifndef __MEDIUM_HXX__
#define __MEDIUM_HXX__

#include "..\Path\Ray.hxx"
#include "..\Misc\Utils2.hxx"

#define MEDIUM_SURVIVAL_PROB 0.8f

class AbstractMedium
{
public:
	enum MediumFlags
	{
		kIsHomogeneous = 0x0001,
		kHasScattering = 0x0002,
		kHasAttenuation = 0x0004
	};

	enum RaySamplingFlags
	{
		kOriginInMedium = 0x0001,
		kEndInMedium    = 0x0002
	};

	AbstractMedium(unsigned int aMediumFlags, const float aContinuationProb, const float aMeanCosine)
		: mMediumFlags(aMediumFlags), mContinuationProb(aContinuationProb), mMeanCosine(aMeanCosine)
	{
		UPBP_ASSERT(aContinuationProb >= 0 && aContinuationProb <= 1);
		UPBP_ASSERT(aMeanCosine >= -1 && aMeanCosine <= 1);
	}

	virtual Rgb GetAbsorptionCoef(
		const Pos &aPos) const = 0;

	virtual Rgb GetEmissionCoef(
		const Pos &aPos) const = 0;

	virtual Rgb GetScatteringCoef(
		const Pos &aPos) const = 0;

	virtual Rgb GetAttenuationCoef(
		const Pos &aPos) const = 0;

	virtual float GetMeanFreePath(
		const Pos &aPos) const
	{
		return HasAttenuation() ? 1.0f / GetAttenuationCoef(aPos).max() : INFINITY;
	}

	virtual Rgb EvalAttenuation(
		const Ray   &aRay,
		const float aDistMin,
		const float aDistMax) const = 0;

	virtual Rgb EvalEmission(
		const Ray   &aRay,
		const float aDistMin, 
		const float aDistMax) const = 0;

	virtual float SampleRay(
		const Ray   &aRay,
		const float aDistToBoundary,	
		const float aRandom,			
		float       *oPdf,
		const uint  aRaySamplingFlags = 0,
		float       *oRevPdf = NULL) const = 0;

	virtual float RaySamplePdf(
		const Ray   &aRay,
		const float aDistMin,
		const float aDistMax,
		const uint  aRaySamplingFlags = 0,		
		float       *oRevPdf = NULL) const = 0;

	virtual bool IsClear() const = 0;

	virtual float MaxBeamLength() const = 0;

	int IsHomogeneous() const   { return mMediumFlags & kIsHomogeneous; }
	int IsHeterogeneous() const { return !IsHomogeneous() ; }

	int HasScattering() const	{ return mMediumFlags & kHasScattering; }
	int HasAttenuation() const	{ return mMediumFlags & kHasAttenuation; }

	inline static int IsIsotropic(const float aMeanCosine)  { return fabsf(aMeanCosine) < 1e-3f; }
	int IsIsotropic() const  { return AbstractMedium::IsIsotropic(mMeanCosine); }

	inline static int IsAnistropic(const float aMeanCosine)  { return !AbstractMedium::IsIsotropic(aMeanCosine); }
	int IsAnistropic() const { return !IsIsotropic(); }

	inline const float ContinuationProb() const { return mContinuationProb; }
	inline const float MeanCosine() const { return mMeanCosine; }

protected:	
	unsigned int	mMediumFlags;
	float			mContinuationProb;
	float			mMeanCosine;	
};

class HomogeneousMedium : public AbstractMedium
{
public:
	HomogeneousMedium(
		const Rgb   &aAbsorptionCoef,
		const Rgb   &aEmissionCoef,
		const Rgb   &aScatteringCoef,
		const float aContinuationProb,
		const float aMeanCosine = 0) : AbstractMedium( kIsHomogeneous, aContinuationProb, aMeanCosine )
	{
		mAbsorptionCoef = aAbsorptionCoef.absValues();
		mEmissionCoef = aEmissionCoef.absValues();
		mScatteringCoef = aScatteringCoef.absValues();
		
		mAttenuationCoef = mAbsorptionCoef + mScatteringCoef;

		mMinPositiveAttenuationCoefComp() = mAttenuationCoef.r();
		mMinAttenuationCoefIndex = 0;
		if (mAttenuationCoef.g() > 0 && (mMinPositiveAttenuationCoefComp() == 0 || mAttenuationCoef.g() < mMinPositiveAttenuationCoefComp())) mMinPositiveAttenuationCoefComp() = mAttenuationCoef.g(), mMinAttenuationCoefIndex = 1;
		if (mAttenuationCoef.b() > 0 && (mMinPositiveAttenuationCoefComp() == 0 || mAttenuationCoef.b() < mMinPositiveAttenuationCoefComp())) mMinPositiveAttenuationCoefComp() = mAttenuationCoef.b(), mMinAttenuationCoefIndex = 2;
		
		if (mScatteringCoef.r() > 0 || mScatteringCoef.g() > 0 || mScatteringCoef.b() > 0)
			mMediumFlags |= kHasScattering;

		if (mAttenuationCoef.r() > 0 || mAttenuationCoef.g() > 0 || mAttenuationCoef.b() > 0)
			mMediumFlags |= kHasAttenuation;

		if (mMediumFlags & kHasAttenuation)
			mMaxBeamLength = (float)(-log(1e-9) / mAttenuationCoef.max());
		else
			mMaxBeamLength = INFINITY;

		mMFP = mMediumFlags & kHasAttenuation ? 1.0f / mAttenuationCoef.max() : INFINITY;
	}

	virtual Rgb GetAbsorptionCoef(
		const Pos &/*aPos*/) const
	{
		return mAbsorptionCoef;
	}

	inline const Rgb& GetAbsorptionCoef() const
	{
		return mAbsorptionCoef;
	}

	virtual Rgb GetEmissionCoef(
		const Pos &/*aPos*/) const
	{
		return mEmissionCoef;
	}

	inline const Rgb& GetEmissionCoef() const
	{
		return mEmissionCoef;
	}

	virtual Rgb GetScatteringCoef(
		const Pos &/*aPos*/) const
	{
		return mScatteringCoef;
	}

	inline const Rgb& GetScatteringCoef() const
	{
		return mScatteringCoef;
	}

	virtual Rgb GetAttenuationCoef(
		const Pos &/*aPos*/) const
	{
		return mAttenuationCoef;
	}

	inline const Rgb& GetAttenuationCoef() const
	{
		return mAttenuationCoef;
	}

	virtual float GetMeanFreePath(
		const Pos &/*aPos*/) const
	{
		return mMFP;
	}

	inline float GetMeanFreePath() const
	{
		return mMFP;
	}

	inline Rgb EvalAttenuation(const float dist) const
	{
		UPBP_ASSERT( dist>=0 );
		return Rgb::exp(-mAttenuationCoef * dist);
	}

	virtual Rgb EvalAttenuation(
		const Ray   &/*aRay*/,
		const float aDistMin, 
		const float aDistMax) const
	{		
		float dist = aDistMax - aDistMin;
		return EvalAttenuation(dist);
	}

	virtual Rgb EvalEmission(
		const Ray   &/*aRay*/,
		const float aDistMin, 
		const float aDistMax) const
	{		
		return mEmissionCoef * (aDistMax - aDistMin);
	}

	// Samples the medium along the given ray starting at its origin. Returns distance along ray to sampled point in media or distance to boundary if sample fell behind
	virtual float SampleRay(
		const Ray   &/*aRay*/,
		const float aDistToBoundary,	
		const float aRandom,			
		float       *oPdf,
		const uint  aRaySamplingFlags = 0,
		float       *oRevPdf = NULL) const
	{			
		UPBP_ASSERT(aDistToBoundary >= 0);
		UPBP_ASSERT(aRandom > 0 && aRandom <= 1);

		if (mMinPositiveAttenuationCoefComp() && HasScattering()) // we can sample along the ray
		{
			float s = -std::log(aRandom) / mMinPositiveAttenuationCoefComp();

			if (s < aDistToBoundary) // sample is before the boundary intersection
			{
				float att = EvalAttenuationInOneDim(mMinPositiveAttenuationCoefComp(), s);

				if (oPdf) *oPdf = mMinPositiveAttenuationCoefComp() * att;

				if (oRevPdf) 
				{
					if (aRaySamplingFlags & kOriginInMedium) *oRevPdf = *oPdf;
					else *oRevPdf = att;
				}

				return s;
			}
			else // sample is behind the boundary intersection
			{
				float att = EvalAttenuationInOneDim(mMinPositiveAttenuationCoefComp(), aDistToBoundary);
				
				if (oPdf) *oPdf = att;
					
				if (oRevPdf) 
				{
					if (aRaySamplingFlags & kOriginInMedium) *oRevPdf = mMinPositiveAttenuationCoefComp() * att;
					else *oRevPdf = att;
				}

				return aDistToBoundary;
			}			
		}
		else // we cannot sample along the ray
		{			
			if (oPdf) *oPdf = 1.0f;
			if (oRevPdf) *oRevPdf = 1.0f;
			return aDistToBoundary;
		}	
	}

	// Get PDF (and optionally reverse PDF) of sampling in the medium along the given ray. Sampling starts at the given min distance and ends at the max distance.
	// If end is said to be inside the medium, PDF for sampling in medium is returned, otherwise PDF for sampling behind the medium is returned.
	virtual float RaySamplePdf(
		const Ray   &/*aRay*/,
		const float aDistMin,
		const float aDistMax,
		const uint  aRaySamplingFlags = 0,		
		float       *oRevPdf = NULL) const
	{
		UPBP_ASSERT(aDistMin >= 0);
		UPBP_ASSERT(aDistMax >= aDistMin);
		
		float oPdf = 1.0f;
		if (oRevPdf) *oRevPdf = 1.0f;
		
		if (mMinPositiveAttenuationCoefComp() && HasScattering()) // we can sample along the ray
		{
			float att = std::max(EvalAttenuationInOneDim(mMinPositiveAttenuationCoefComp(), aDistMax - aDistMin),1e-35f);
			float minatt = mMinPositiveAttenuationCoefComp() * att;

			if (aRaySamplingFlags & kEndInMedium) oPdf =  minatt;
			else oPdf = att;

			if (oRevPdf)
			{
				if (aRaySamplingFlags & kOriginInMedium) *oRevPdf =  minatt;
				else *oRevPdf = att;
			}
		}
		
		return oPdf;
	}

	virtual float MaxBeamLength() const { return mMaxBeamLength; }
	
	inline const float mMinPositiveAttenuationCoefComp() const { return mAttenuationCoef.extraData(); }
	inline const int mMinPositiveAttenuationCoefCompIndex() const { return mMinAttenuationCoefIndex; }
	virtual bool IsClear() const { return false; }

protected:
	Rgb   mAbsorptionCoef;
	Rgb   mEmissionCoef;
	Rgb   mScatteringCoef;
	Rgb   mAttenuationCoef;
	int	  mMinAttenuationCoefIndex;
	float mMaxBeamLength;
	float mMFP;

	inline float& mMinPositiveAttenuationCoefComp() { return mAttenuationCoef.extraData(); }

private:
	float EvalAttenuationInOneDim(
		const float aAttenuationCoefComp,
		const float aDistanceAlongRay) const
	{
		return std::exp(-aAttenuationCoefComp* aDistanceAlongRay);
	}
	
};

class ClearMedium : public HomogeneousMedium
{
public:
	ClearMedium() : HomogeneousMedium(Rgb(0.0f), Rgb(0.0f), Rgb(0.0f), 1.0f) {}

	virtual bool IsClear() const { return true; }

	virtual float MaxBeamLength() const { return INFINITY; }
};

#endif //__MEDIUM_HXX__