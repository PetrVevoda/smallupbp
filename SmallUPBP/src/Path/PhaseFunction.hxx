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

#ifndef __PHASEFUNTION_HXX__
#define __PHASEFUNTION_HXX__

#include "Frame.hxx"
#include "..\Scene\Media.hxx"

class PhaseFunction
{
public:
	static Rgb Evaluate(
        const Dir   &aWorldDirFix, // Points away from the scattering location
        const Dir   &aWorldDirGen, // Points away from the scattering location
        const float aMeanCosine,
        float       *oDirectPdfW = NULL,
        float       *oReversePdfW = NULL,
		float       *oSinTheta = NULL)
	{
		float pdf = PhaseFunction::Pdf(aWorldDirFix, aWorldDirGen, aMeanCosine, oSinTheta);
		
		if (oDirectPdfW) *oDirectPdfW = pdf;
		if (oReversePdfW) *oReversePdfW = pdf;
		
		return Rgb(pdf);
	}

	static float Pdf(
		const Dir   &aWorldDirFix, // Points away from the scattering location
        const Dir   &aWorldDirGen, // Points away from the scattering location
        const float aMeanCosine,
		float       *oSinTheta = NULL)
	{
		UPBP_ASSERT(aMeanCosine >= -1 && aMeanCosine <= 1);
		UPBP_ASSERT(aWorldDirFix.isRoughlyNormalized() && aWorldDirGen.isRoughlyNormalized());

		if (AbstractMedium::IsIsotropic(aMeanCosine))
		{
			if (oSinTheta)
			{
				const float cosTheta = -dot(aWorldDirGen, aWorldDirFix);
				*oSinTheta = sqrtf(std::max(0.f, 1.f - cosTheta * cosTheta));
			}
			return UniformSpherePdfW();
		}
		else
		{			
			const float cosTheta = -dot(aWorldDirGen, aWorldDirFix);
			const float squareMeanCosine = aMeanCosine * aMeanCosine;
			const float d = 1.f + squareMeanCosine - (aMeanCosine + aMeanCosine) * cosTheta;
			if (oSinTheta)
				*oSinTheta = sqrtf(std::max(0.f, 1.f - cosTheta * cosTheta));

			return d > 0.f ? ( UniformSpherePdfW() * (1.f - squareMeanCosine) / (d * sqrtf(d)) ) : 0.f; 
		}
	}

	static Rgb Sample(		
		const Dir   &aWorldDirFix, // Points away from the scattering location
		const float aMeanCosine,
        const Dir   &aRndTriplet,
        Dir         &oWorldDirGen, // Points away from the scattering location
        float       &oPdfW,
		float       *oSinTheta = NULL)
	{
		Frame frame;
	    frame.SetFromZ(-aWorldDirFix);
		return Sample(aWorldDirFix, aMeanCosine, aRndTriplet, frame, oWorldDirGen, oPdfW, oSinTheta);
	}

	static Rgb Sample(		
		const Dir   &aWorldDirFix, // Points away from the scattering location
		const float aMeanCosine,
        const Dir   &aRndTriplet,
		const Frame &aFrame,
        Dir         &oWorldDirGen, // Points away from the scattering location
        float       &oPdfW,
		float       *oSinTheta = NULL)
	{
		UPBP_ASSERT(aMeanCosine >= -1 && aMeanCosine <= 1);
		UPBP_ASSERT(aWorldDirFix.isRoughlyNormalized());
		
		if (AbstractMedium::IsIsotropic(aMeanCosine))
		{
			oWorldDirGen = SampleUniformSphereW(Vec2f(aRndTriplet.x(), aRndTriplet.y()), &oPdfW);
			if (oSinTheta)
			{
				const float cosTheta = -dot(oWorldDirGen, aWorldDirFix);
				*oSinTheta = sqrtf(std::max(0.f, 1.f - cosTheta * cosTheta));
			}
		}
		else
		{
			const float squareMeanCosine = aMeanCosine * aMeanCosine;
			const float twoCosine = aMeanCosine + aMeanCosine;
            const float sqrtt = (1.f - squareMeanCosine) / (1.f - aMeanCosine + twoCosine * aRndTriplet.x());
            const float cosTheta = (1.f + squareMeanCosine - sqrtt * sqrtt) / twoCosine;
            const float sinTheta = sqrtf(std::max(0.f, 1.f - cosTheta * cosTheta));
            const float phi = 2.f * PI_F * aRndTriplet.y();            
			const float sinPhi = sinf(phi);
			const float cosPhi = cosf(phi);
			const float d = 1.f + squareMeanCosine - twoCosine * cosTheta;

			oWorldDirGen = aFrame.ToWorld(Dir(cosPhi * sinTheta, sinPhi * sinTheta, cosTheta));
			oPdfW = d > 0.f ? ( UniformSpherePdfW() * (1.f - squareMeanCosine) / (d * sqrtf(d)) ) : 0.f;
			if (oSinTheta)
				*oSinTheta = sinTheta;
		}

		UPBP_ASSERT(oWorldDirGen.isRoughlyNormalized());

		return Rgb(oPdfW);
	}
};

#endif //__PHASEFUNTION_HXX__