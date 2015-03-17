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

#ifndef __BSDF_HXX__
#define __BSDF_HXX__

#include <vector>
#include <cmath>

#include "PhaseFunction.hxx"
#include "..\Scene\Scene.hxx"

#define STRICT_NORMALS

//////////////////////////////////////////////////////////////////////////
// BSDF, most magic happens here
//
// One of important conventions is prefixing direction with World when
// are in world coordinates and with Local when they are in local frame,
// i.e., mFrame.
//
// Another important convention if suffix Fix and Gen.
// For PDF computation, we need to know which direction is given (Fix),
// and which is the generated (Gen) direction. This is important even
// when simply evaluating BSDF.
// In BPT, we call Evaluate() when directly connecting to light/camera.
// This gives us both directions required for evaluating BSDF.
// However, for MIS we also need to know probabilities of having sampled
// this path via BSDF sampling, and we need that for both possible directions.
// The Fix/Gen convention (along with Direct and Reverse for PDF) clearly
// establishes which PDF is which.
// 
// Used both for events on surface as well as in medium. Depending on constructor
// parameters BSDF knows, if it is on surface or in medium. Then each of its
// methods begin with if and continue with appropriate code. Surface events are
// handled directly in this class, for scattering in media PhaseFunction methods
// are called.

class BSDF
{
public:
	enum Events
    {
        kNONE        = 0,
        kDiffuse     = 1,
        kPhong       = 2,
        kReflect     = 4,
        kRefract     = 8,
		kScatter     = 16, // media
        kSpecular    = (kReflect  | kRefract),
        kNonSpecular = (kDiffuse  | kPhong | kScatter),
        kAll         = (kSpecular | kNonSpecular)
    };

	enum Direction
	{
		kFromCamera,
		kFromLight
	};

	enum PdfDir
	{
		kForward,
		kReverse
	};

    BSDF():mData2(0.0f){};

    BSDF(
        const Ray       &aRay,
        const Isect     &aIsect,
        const Scene     &aScene,
		const Direction aDirection = kFromCamera,
		const float     aIOR = -1.0f)
	{
		Setup(aRay, aIsect, aScene, aDirection, aIOR);
	}    

    void Setup(
        const Ray       &aRay,
        const Isect     &aIsect,
        const Scene     &aScene,
		const Direction aDirection,
		const float     aIOR)
    {
		UPBP_ASSERT(aRay.direction.isRoughlyNormalized());
		
		mFlags() = 0;
		Flags flags = 0;

		if (aDirection == kFromLight) flags |= kFromLightBit;
		
		if (aIsect.IsOnSurface())
		{
			const Material &mat = aScene.GetMaterial(aIsect.mMatID);
			setMaterialPtr(&mat);
			mFrame().SetFromZ(aIsect.mShadingNormal);
			mLocalDirFix() = mFrame().ToLocal(-aRay.direction);

#ifdef STRICT_NORMALS
			mGeometryNormal() = mFrame().ToLocal(aIsect.mNormal);
			mCosThetaFixGeom() = dot(mLocalDirFix(), mGeometryNormal());
#endif		

			// Reject rays that are too parallel with tangent plane
			if (std::abs(mCosThetaFix()) < EPS_COSINE
#ifdef STRICT_NORMALS
				|| std::abs(mCosThetaFixGeom()) < EPS_COSINE
#endif
				)
			{
				mFlags() = 0;
				return;
			}				

			mIOR() = aIOR;

			GetComponentProbabilities(mat);
			if ((mDiffProb() == 0) && (mPhongProb() == 0)) flags |= kDeltaBit;			
		}
		else
		{			
			flags |= kInMediumBit;
			setMediumPtr(aScene.GetMediumPtr(aIsect.mMedID));
			mWorldDirFix() = -aRay.direction;
			mFrame().SetFromZ(-mWorldDirFix());
			mContinuationProb() = mMediumPtr()->ContinuationProb();
			mMeanCosine() = mMediumPtr()->MeanCosine();
			mScatterCoef() = mMediumPtr()->GetScatteringCoef(aRay.origin + aRay.direction * aIsect.mDist);
			mScatterCoefPos() = mScatterCoef().isBlackOrNegative() ? 0 : 1;
		}

		flags |= kValidBit;
		mFlags() = flags;
    }

    /* \brief Given a direction, evaluates BSDF
     *
     * Returns value of BSDF, as well as cosine for the
     * aWorldDirGen direction.
     * Can return probability (w.r.t. solid angle W),
     * of having sampled aWorldDirGen given mLocalDirFix (oDirectPdfW),
     * and of having sampled mLocalDirFix given aWorldDirGen (oReversePdfW).
     *
     */
    Rgb Evaluate(
        const Dir   &aWorldDirGen, // Points away from the scattering location
        float       &oCosThetaGen,
        float       *oDirectPdfW = NULL,
        float       *oReversePdfW = NULL,
		float       *oSinTheta = NULL) const
    {
		if (IsOnSurface()) // surface
		{
			Rgb result(0);
			
			if (oDirectPdfW)  *oDirectPdfW = 0;
			if (oReversePdfW) *oReversePdfW = 0;
			if (oSinTheta)    *oSinTheta = 0;

			const Dir localDirGen = mFrame().ToLocal(aWorldDirGen);
			
			const float cosThetaGen     = localDirGen.z();
#ifdef STRICT_NORMALS
			const float cosThetaGenGeom = dot(localDirGen, mGeometryNormal());
#endif

			oCosThetaGen = std::abs(cosThetaGen);

			// Samples too parallel with tangent plane are rejected
			if (oCosThetaGen < EPS_COSINE 
#ifdef STRICT_NORMALS
				|| fabsf(cosThetaGenGeom) < EPS_COSINE
#endif
				)
				return result;

			// Generated direction must point to the same side from the surface as the fixed one (potential refraction has zero PDF anyway since it is delta)
			if(cosThetaGen * mCosThetaFix() < 0 
#ifdef STRICT_NORMALS
				|| cosThetaGenGeom * mCosThetaFixGeom() < 0 
#endif
				)
				return result;			

			const Material& mat = *mMaterialPtr();

			result += EvaluateDiffuse(mat, localDirGen, oDirectPdfW, oReversePdfW);
			result += EvaluatePhong(mat, localDirGen, oDirectPdfW, oReversePdfW);

			return result;
		}
		else // medium
		{
			oCosThetaGen = 1.0f;

			// No need to evaluate phase function if the scattering coef is zero
			if (!mScatterCoefPos())
			{
				if (oDirectPdfW)  *oDirectPdfW = 0;
				if (oReversePdfW) *oReversePdfW = 0;
				if (oSinTheta)    *oSinTheta = 0;
				return Rgb(0);
			}
			else
				return mScatterCoef() * PhaseFunction::Evaluate(mWorldDirFix(), aWorldDirGen, mMeanCosine(), oDirectPdfW, oReversePdfW, oSinTheta);
		}       
	}

    /* \brief Given a direction, evaluates PDF
     *
     * By default returns PDF with which would be aWorldDirGen
     * generated from mLocalDirFix. When aPdfDir == kReverse,
     * it provides PDF for the reverse direction.
     */
    float Pdf(
        const Dir    &aWorldDirGen, // Points away from the scattering location
		const PdfDir aPdfDir = kForward) const
    {
       	if (IsOnSurface()) // surface
		{
			const Dir localDirGen = mFrame().ToLocal(aWorldDirGen);

			const float cosThetaGen     = localDirGen.z();
#ifdef STRICT_NORMALS
			const float cosThetaGenGeom = dot(localDirGen, mGeometryNormal());
#endif
			// Samples too parallel with tangent plane are rejected
			if (fabsf(cosThetaGen) < EPS_COSINE 
#ifdef STRICT_NORMALS
				|| fabsf(cosThetaGenGeom) < EPS_COSINE
#endif
				)
				return 0;
			
			// Generated direction must point to the same side from the surface as the fixed one (potential refraction has zero PDF anyway since it is delta)
			if(cosThetaGen * mCosThetaFix() < 0 
#ifdef STRICT_NORMALS
				|| cosThetaGenGeom * mCosThetaFixGeom() < 0 
#endif
				)
				return 0;			

			float directPdfW  = 0;
			float reversePdfW = 0;

			const Material& mat = *mMaterialPtr();

			PdfDiffuse(localDirGen, &directPdfW, &reversePdfW);
			PdfPhong(mat, localDirGen, &directPdfW, &reversePdfW);

			return aPdfDir == kReverse ? reversePdfW : directPdfW;
		}
		else // medium
		{
			return PhaseFunction::Pdf(mWorldDirFix(), aWorldDirGen, mMeanCosine());
		}
	}

    /* \brief Given 3 random numbers, samples new direction from BSDF.
     *
     * Uses z component of random triplet to pick BSDF component from
     * which it will sample direction. If non-specular component is chosen,
     * it will also evaluate the other (non-specular) BSDF components.
     * Return BSDF factor for given direction, as well as PDF choosing that direction.
     * Can return event which has been sampled.
     * If result is Dir(0,0,0), then the sample should be discarded.
     */
    Rgb Sample(
        const Dir   &aRndTriplet,
        Dir         &oWorldDirGen, // Points away from the scattering location
        float       &oPdfW,
        float       &oCosThetaGen,
        uint        *oSampledEvent = NULL,
		float       *oSinTheta = NULL) const
	{
		if (IsOnSurface()) // surface
		{
			uint sampledEvent;

			if(aRndTriplet.z() < mDiffProb())
				sampledEvent = kDiffuse;
			else if(aRndTriplet.z() < mDiffProb() + mPhongProb())
				sampledEvent = kPhong;
			else if(aRndTriplet.z() < mDiffProb() + mPhongProb() + mReflProb())
				sampledEvent = kReflect;
			else
				sampledEvent = kRefract;

			if(oSampledEvent)
				*oSampledEvent = sampledEvent;
			if (oSinTheta) 
				*oSinTheta = 0;

			oPdfW = 0;
			Rgb result(0);
			Dir localDirGen;

			const Material& mat = *mMaterialPtr();

			if(sampledEvent == kDiffuse)
			{
				result += SampleDiffuse(mat, Vec2f(aRndTriplet[0], aRndTriplet[1]), localDirGen, oPdfW);

				if(result.isBlackOrNegative())
					return Rgb(0);

				result += EvaluatePhong(mat, localDirGen, &oPdfW);
			}
			else if(sampledEvent == kPhong)
			{
				result += SamplePhong(mat, Vec2f(aRndTriplet[0], aRndTriplet[1]), localDirGen, oPdfW);

				if(result.isBlackOrNegative())
					return Rgb(0);

				result += EvaluateDiffuse(mat, localDirGen, &oPdfW);
			}
			else if(sampledEvent == kReflect)
			{
				result += SampleReflect(mat, Vec2f(aRndTriplet[0], aRndTriplet[1]), localDirGen, oPdfW);

				if(result.isBlackOrNegative())
					return Rgb(0);
			}
			else
			{
				result += SampleRefract(mat, Vec2f(aRndTriplet[0], aRndTriplet[1]), localDirGen, oPdfW);
				if(result.isBlackOrNegative())
					return Rgb(0);
			}

			const float cosThetaGen     = localDirGen.z();
#ifdef STRICT_NORMALS
			const float cosThetaGenGeom = dot(localDirGen, mGeometryNormal());
#endif

			oCosThetaGen = std::abs(cosThetaGen);

			oWorldDirGen = mFrame().ToWorld(localDirGen);

			// Reject samples that are too parallel with tangent plane
			if (oCosThetaGen < EPS_COSINE 
#ifdef STRICT_NORMALS
				|| fabsf(cosThetaGenGeom) < EPS_COSINE
#endif
				)
				return Rgb(0.f);

			// Refraction must cross the surface, other interactions must not
			if ((sampledEvent == kRefract && cosThetaGen * mCosThetaFix() > 0) ||
				(sampledEvent != kRefract && cosThetaGen * mCosThetaFix() < 0)
#ifdef STRICT_NORMALS
			 || (sampledEvent == kRefract && cosThetaGenGeom * mCosThetaFixGeom() > 0) ||
				(sampledEvent != kRefract && cosThetaGenGeom * mCosThetaFixGeom() < 0)
#endif				
				)
				return Rgb(0.f);

			UPBP_ASSERT(oWorldDirGen.isRoughlyNormalized());

			return result;
		}
		else // medium
		{
			oCosThetaGen = 1.0f;
			if(oSampledEvent) *oSampledEvent = kScatter;

			// No need to evaluate phase function if the scattering coef is zero
			if (!mScatterCoefPos())
			{				
				oWorldDirGen = Dir(0);
				oPdfW = 0;
				if (oSinTheta) *oSinTheta = 0;
				return Rgb(0);
			}
			else return mScatterCoef() * PhaseFunction::Sample(mWorldDirFix(), mMeanCosine(), aRndTriplet, mFrame(), oWorldDirGen, oPdfW, oSinTheta);
		}
    }

	inline const bool IsValid() const            { return (mFlags() & kValidBit) != 0;     }
    inline const bool IsDelta() const            { return (mFlags() & kDeltaBit) != 0;     }
	inline const bool IsOnSurface() const        { return !IsInMedium();                   }
	inline const bool IsInMedium() const         { return (mFlags() & kInMediumBit) != 0;  }
	inline const bool IsFromCamera() const       { return !IsFromLight();                  }
	inline const bool IsFromLight() const        { return (mFlags() & kFromLightBit) != 0; }
    inline const float ContinuationProb() const  { return mContinuationProb();             }
    inline const float CosThetaFix() const       { return IsOnSurface() ? mCosThetaFix() : 1.0f; }
	inline const Dir WorldDirFix() const         { return IsOnSurface() ? mFrame().ToWorld(mLocalDirFix()) : mWorldDirFix(); }
	inline const Material* GetMaterial() const      { return IsOnSurface() ? mMaterialPtr() : nullptr; }
	inline const AbstractMedium* GetMedium() const  { return IsOnSurface() ? nullptr : mMediumPtr();   }

private:

    ////////////////////////////////////////////////////////////////////////////
    // Sampling methods
    // All sampling methods take material, 2 random numbers [0-1],
    // and return BSDF factor, generated direction in local coordinates, and PDF
    ////////////////////////////////////////////////////////////////////////////

    Rgb SampleDiffuse(
		const Material &aMaterial,
        const Vec2f    &aRndTuple,
        Dir            &oLocalDirGen,
        float          &oPdfW) const
    {
        if(mLocalDirFix().z() < EPS_COSINE)
            return Rgb(0);

        float unweightedPdfW;
        oLocalDirGen = SampleCosHemisphereW(aRndTuple, &unweightedPdfW);
        oPdfW += unweightedPdfW * mDiffProb();

        return aMaterial.mDiffuseReflectance * INV_PI_F;
    }

    Rgb SamplePhong(
		const Material &aMaterial,
        const Vec2f    &aRndTuple,
        Dir            &oLocalDirGen,
        float          &oPdfW) const
    {		
		oLocalDirGen = SamplePowerCosHemisphereW(aRndTuple, aMaterial.mPhongExponent, NULL);

        // Due to numeric issues in MIS, we actually need to compute all PDFs
        // exactly the same way all the time!!!
        const Dir reflLocalDirFixed = ReflectLocal(mLocalDirFix());
        {
            Frame frame;
            frame.SetFromZ(reflLocalDirFixed);
            oLocalDirGen = frame.ToWorld(oLocalDirGen);
        }

        const float dot_R_Wi = dot(reflLocalDirFixed, oLocalDirGen);

        if(dot_R_Wi <= EPS_PHONG)
            return Rgb(0.f);

        PdfPhong(aMaterial, oLocalDirGen, &oPdfW);

        const Rgb rho = aMaterial.mPhongReflectance *
            (aMaterial.mPhongExponent + 2.f) * 0.5f * INV_PI_F;

        return rho * std::pow(dot_R_Wi, aMaterial.mPhongExponent);
    }

    Rgb SampleReflect(
		const Material &aMaterial,
        const Vec2f    &aRndTuple,
        Dir            &oLocalDirGen,
        float          &oPdfW) const
    {
        oLocalDirGen = ReflectLocal(mLocalDirFix());

        oPdfW += mReflProb();
        
		// BSDF is multiplied (outside) by cosine (oLocalDirGen.z()),
        // for mirror this shouldn't be done, so we pre-divide here instead
        return mReflectCoeff() * aMaterial.mMirrorReflectance /
            std::abs(oLocalDirGen.z());
    }

    Rgb SampleRefract(
		const Material &aMaterial,
        const Vec2f    &aRndTuple,
        Dir            &oLocalDirGen,
        float          &oPdfW) const
    {
        if(mIOR() < 0)
            return Rgb(0);

        float cosI = mLocalDirFix().z();

        float cosT;
        float etaIncOverEtaTrans = mIOR();

        if(cosI < 0.f) // hit from inside
        {
            cosI = -cosI;
            cosT = 1.f;
        }
        else
        {
            cosT = -1.f;
        }

        const float sinI2 = 1.f - cosI * cosI;
        const float sinT2 = Utils::sqr(etaIncOverEtaTrans) * sinI2;

        if(sinT2 < 1.f) // no total internal reflection
        {
            cosT *= std::sqrt(std::max(0.f, 1.f - sinT2));

            oLocalDirGen = Dir(
                -etaIncOverEtaTrans * mLocalDirFix().x(),
                -etaIncOverEtaTrans * mLocalDirFix().y(),
                cosT);

            oPdfW += mRefrProb();

            const float refractCoeff = 1.f - mReflectCoeff();
            
			// only camera paths are multiplied by this factor, and etas
            // are swapped because radiance flows in the opposite direction
			if(!IsFromLight())
                return Rgb(refractCoeff * Utils::sqr(etaIncOverEtaTrans) / std::abs(cosT));
            else
                return Rgb(refractCoeff / std::abs(cosT));
        }
        //else total internal reflection, do nothing

        oPdfW += 0.f;
        return Rgb(0.f);
    }

    ////////////////////////////////////////////////////////////////////////////
    // Evaluation methods
    ////////////////////////////////////////////////////////////////////////////

    Rgb EvaluateDiffuse(
		const Material &aMaterial,
        const Dir      &aLocalDirGen,
        float          *oDirectPdfW = NULL,
        float          *oReversePdfW = NULL) const
    {
        if(mDiffProb() == 0)
            return Rgb(0);

        if(mLocalDirFix().z() < EPS_COSINE || aLocalDirGen.z() < EPS_COSINE)
            return Rgb(0);

        if(oDirectPdfW)
            *oDirectPdfW += mDiffProb() * std::max(0.f, aLocalDirGen.z() * INV_PI_F);

        if(oReversePdfW)
            *oReversePdfW += mDiffProb() * std::max(0.f, mLocalDirFix().z() * INV_PI_F);

        return aMaterial.mDiffuseReflectance * INV_PI_F;
    }

    Rgb EvaluatePhong(
		const Material &aMaterial,
        const Dir      &aLocalDirGen,
        float          *oDirectPdfW = NULL,
        float          *oReversePdfW = NULL) const
    {
        if(mPhongProb() == 0)
            return Rgb(0);

        if(mLocalDirFix().z() < EPS_COSINE || aLocalDirGen.z() < EPS_COSINE)
            return Rgb(0);

        // assumes this is never called when rejectShadingCos(oLocalDirGen.z()) is true
        const Dir reflLocalDirIn = ReflectLocal(mLocalDirFix());
        const float dot_R_Wi = dot(reflLocalDirIn, aLocalDirGen);

        if(dot_R_Wi <= EPS_PHONG)
            return Rgb(0.f);

		float pow = std::pow(dot_R_Wi, aMaterial.mPhongExponent);
		if (!Float::isPositive(pow))
			return Rgb(0.f);

        if(oDirectPdfW || oReversePdfW)
        {
            // the sampling is symmetric
            const float pdfW = mPhongProb() *
                PowerCosHemispherePdfW(reflLocalDirIn, aLocalDirGen, aMaterial.mPhongExponent);

            if(oDirectPdfW)
                *oDirectPdfW  += pdfW;

            if(oReversePdfW)
                *oReversePdfW += pdfW;
        }

        const Rgb rho = aMaterial.mPhongReflectance *
            (aMaterial.mPhongExponent + 2.f) * 0.5f * INV_PI_F;

        return rho * pow;
    }

    ////////////////////////////////////////////////////////////////////////////
    // PDF methods
    ////////////////////////////////////////////////////////////////////////////

    void PdfDiffuse(
        const Dir      &aLocalDirGen,
        float          *oDirectPdfW = NULL,
        float          *oReversePdfW = NULL) const
    {
        if(mDiffProb() == 0)
            return;

        if(oDirectPdfW)
            *oDirectPdfW  += mDiffProb() *
            std::max(0.f, aLocalDirGen.z() * INV_PI_F);

        if(oReversePdfW)
            *oReversePdfW += mDiffProb() *
            std::max(0.f, mLocalDirFix().z() * INV_PI_F);
    }

    void PdfPhong(
		const Material &aMaterial,
        const Dir      &aLocalDirGen,
        float          *oDirectPdfW = NULL,
        float          *oReversePdfW = NULL) const
    {
        if(mPhongProb() == 0)
            return;

        // assumes this is never called when rejectShadingCos(oLocalDirGen.z()) is true
        const Dir reflLocalDirIn = ReflectLocal(mLocalDirFix());
        const float dot_R_Wi = dot(reflLocalDirIn, aLocalDirGen);

        if(dot_R_Wi <= EPS_PHONG)
            return;

        if(oDirectPdfW || oReversePdfW)
        {
            // the sampling is symmetric
            const float pdfW = PowerCosHemispherePdfW(reflLocalDirIn, aLocalDirGen,
                aMaterial.mPhongExponent) * mPhongProb();

            if(oDirectPdfW)
                *oDirectPdfW  += pdfW;

            if(oReversePdfW)
                *oReversePdfW += pdfW;
        }
    }

    ////////////////////////////////////////////////////////////////////////////
    // Albedo methods
    ////////////////////////////////////////////////////////////////////////////

    float AlbedoDiffuse(const Material& aMaterial) const
    {
        return Luminance(aMaterial.mDiffuseReflectance);
    }

    float AlbedoPhong(const Material& aMaterial) const
    {
        return Luminance(aMaterial.mPhongReflectance);
    }

    float AlbedoReflect(const Material& aMaterial) const
    {
        return Luminance(aMaterial.mMirrorReflectance);
    }

    float AlbedoRefract(const Material& aMaterial) const
    {
        return 1.f;
    }

    void GetComponentProbabilities(const Material &aMaterial)
    {
        mReflectCoeff() = FresnelDielectric(mLocalDirFix().z(), mIOR());

        const float albedoDiffuse = AlbedoDiffuse(aMaterial);
        const float albedoPhong   = AlbedoPhong(aMaterial);
        const float albedoReflect = mReflectCoeff()         * AlbedoReflect(aMaterial);
        const float albedoRefract = (1.f - mReflectCoeff()) * AlbedoRefract(aMaterial);

        const float totalAlbedo = albedoDiffuse + albedoPhong + albedoReflect + albedoRefract;

        if(totalAlbedo < 1e-9f)
        {
            mDiffProb()  = 0.f;
            mPhongProb() = 0.f;
            mReflProb()  = 0.f;
            mRefrProb()  = 0.f;
            mContinuationProb() = 0.f;
        }
        else
        {
            mDiffProb()  = albedoDiffuse / totalAlbedo;
            mPhongProb() = albedoPhong   / totalAlbedo;
            mReflProb()  = albedoReflect / totalAlbedo;
            mRefrProb()  = albedoRefract / totalAlbedo;

            // The continuation probability is max component from reflectance.
            // That way the weight of sample will never rise.
            // Luminance is another very valid option.
            mContinuationProb() =
                (aMaterial.mDiffuseReflectance +
                aMaterial.mPhongReflectance +
                mReflectCoeff() * aMaterial.mMirrorReflectance).max() +
                (1.f - mReflectCoeff());

            mContinuationProb() = std::min(1.f, std::max(0.f, mContinuationProb()));
        }
    }

private:
	enum FlagBits
	{
		kInMediumBit  = 0x0001, //!< Set if BSDF is in medium, not on surface
		kFromLightBit = 0x0002, //!< Set if ray comes from light, not from camera
		kValidBit     = 0x0004, //!< Set when BSDF is valid
		kDeltaBit     = 0x0008  //!< Set when material/medium is purely specular
	};

	typedef int Flags;

	union {
		const Material*       mat;
		const AbstractMedium* med;
	} mData0;
	inline const Material*       mMaterialPtr() const { return mData0.mat;          } //!< Material of the surface in the BSDF location [surface]
	inline const AbstractMedium* mMediumPtr()   const { return mData0.med;          } //!< Medium in the BSDF location [medium]
	inline void setMaterialPtr(const Material* aMaterialPtr)   { mData0.mat = aMaterialPtr; }
	inline void setMediumPtr(const AbstractMedium* aMediumPtr) { mData0.med = aMediumPtr;   }

	Frame mData1;
	inline const Frame& mFrame()            const { return mData1;	                } //!< Local frame of reference
	inline Frame&       mFrame()                  { return mData1;	                }
	inline const float  mContinuationProb() const { return mData1.mX.extraData();	} //!< Russian roulette probability
	inline float&       mContinuationProb()       { return mData1.mX.extraData();	}
	inline const float  mReflectCoeff()     const { return mData1.mY.extraData();	} //!< Fresnel reflection coefficient (for glass) [surface]
	inline float&       mReflectCoeff()           { return mData1.mY.extraData();   }
	inline const float  mMeanCosine()       const { return mData1.mY.extraData();	} //!< Coefficient of anisotropy                  [medium]
	inline float&       mMeanCosine()             { return mData1.mY.extraData();   }
	inline const float  mIOR()              const { return mData1.mZ.extraData();	} //!< Relative index of refraction [surface]
	inline float&       mIOR()                    { return mData1.mZ.extraData();   }
	
	Dir   mData2;
	inline const Dir&   mLocalDirFix()      const { return mData2;	                } //!< Incoming (fixed) direction, in local	[surface]
	inline Dir&         mLocalDirFix()            { return mData2;	                }
	inline const Dir&   mWorldDirFix()      const { return mData2;	                } //!< Incoming (fixed) direction, in world	[medium]
	inline Dir&         mWorldDirFix()            { return mData2;	                }
	inline const Flags  mFlags()            const { return mData2.extraDataInt();   } //!< BSDF flags
	inline Flags&       mFlags()                  { return mData2.extraDataInt();   }
	inline const float  mCosThetaFix()      const { return mData2.z();	            } //!< Cosine of angle between the incoming direction and the shading normal [surface]
	
	Rgb   mData3;
	inline const Rgb&   mScatterCoef()      const { return mData3;	                } //!< Medium scattering coefficient [medium]
	inline Rgb&         mScatterCoef()            { return mData3;	                }
	inline const float  mDiffProb()         const { return mData3.r();	            } //!< Sampling probabilities [surface]
	inline float&       mDiffProb()               { return mData3.r();	            }
	inline const float  mPhongProb()        const { return mData3.g();	            } //!< Sampling probabilities [surface]
	inline float&       mPhongProb()              { return mData3.g();	            }
	inline const float  mReflProb()         const { return mData3.b();	            } //!< Sampling probabilities [surface]
	inline float&       mReflProb()               { return mData3.b();	            }
	inline const float  mRefrProb()         const { return mData3.extraData();      } //!< Sampling probabilities [surface]
	inline float&       mRefrProb()               { return mData3.extraData();      }
	inline const bool   mScatterCoefPos()   const { return mData3.extraData() != 0; } //!< Whether the medium scattering coefficient is positive [medium]
	inline float&       mScatterCoefPos()         { return mData3.extraData();      }

#ifdef STRICT_NORMALS
	Dir  mData4;	
	inline const Dir&   mGeometryNormal()   const { return mData4;				    } //!< Surface geometry normal (in local) [surface]
	inline Dir&         mGeometryNormal()         { return mData4;				    }
	inline const float  mCosThetaFixGeom()  const { return mData4.extraData();      } //!< Cosine of angle between the incoming direction and the geometry normal [surface]
	inline float&       mCosThetaFixGeom()        { return mData4.extraData();      }
#endif
};

#endif //__BSDF_HXX__
