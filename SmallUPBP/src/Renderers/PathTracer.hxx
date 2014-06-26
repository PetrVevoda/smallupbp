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

#ifndef __PATHTRACER_HXX__
#define __PATHTRACER_HXX__

#include <vector>
#include <cmath>

#include "..\Path\Bsdf.hxx"
#include "Renderer.hxx"

class PathTracer : public AbstractRenderer
{
public:

	PathTracer(
		const Scene& aScene,
		int aSeed = 1234
		) :
	AbstractRenderer(aScene), mRng(aSeed)
	{}

	virtual void RunIteration(int aIteration)
	{
		// We sample lights uniformly
		const int   lightCount    = mScene.GetLightCount();
		const float lightPickProb = 1.f / lightCount;

		const int resX = int(mScene.mCamera.mResolution.get(0));
		const int resY = int(mScene.mCamera.mResolution.get(1));

		for(int pixID = 0; pixID < resX * resY; pixID++)
		{
			const int x = pixID % resX;
			const int y = pixID / resX;

			const Vec2f sample = Vec2f(float(x), float(y)) + mRng.GetVec2f();

			Ray ray = mScene.mCamera.GenerateRay(sample);
			Isect isect(1e36f);

			Rgb pathWeight(1.f);
			Rgb color(0.f);
			uint  pathLength   = 1;
			bool  lastSpecular = true;
			float lastPdfW     = 1;

			mScene.InitBoundaryStack(mBoundaryStack);

			for(;; ++pathLength)
			{				
				if(!mScene.Intersect(ray, isect, mBoundaryStack))
				{
					if(pathLength < mMinPathLength)
						break;

					const BackgroundLight* background = mScene.GetBackground();
					if(!background)
						break;
					// For background we cheat with the A/W suffixes,
					// and GetRadiance actually returns W instead of A
					float directPdfW;
					Rgb contrib = background->GetRadiance(mScene.mSceneSphere,
						ray.direction, Pos(0), &directPdfW);
					if(contrib.isBlackOrNegative())
						break;

					float misWeight = 1.f;
					if(pathLength > 1 && !lastSpecular)
					{
						misWeight = Mis2(lastPdfW, directPdfW * lightPickProb);
					}

					color += pathWeight * misWeight * contrib;
					break;
				}

				Pos hitPoint = ray.origin + ray.direction * isect.mDist;

				BSDF bsdf(ray, isect, mScene, BSDF::kFromCamera, mScene.RelativeIOR(isect, mBoundaryStack));
				if(!bsdf.IsValid())
					break;

				// directly hit some light, lights do not reflect
				if(isect.mLightID >= 0)
				{
					if(pathLength < mMinPathLength)
						break;

					const AbstractLight *light = mScene.GetLightPtr(isect.mLightID);
					float directPdfA;
					Rgb contrib = light->GetRadiance(mScene.mSceneSphere,
						ray.direction, hitPoint, &directPdfA);
					if(contrib.isBlackOrNegative())
						break;

					float misWeight = 1.f;
					if(pathLength > 1 && !lastSpecular)
					{
						const float directPdfW = PdfAtoW(directPdfA, isect.mDist,
							bsdf.CosThetaFix());
						misWeight = Mis2(lastPdfW, directPdfW * lightPickProb);
					}

					color += pathWeight * misWeight * contrib;
					break;
				}

				if(pathLength >= mMaxPathLength)
					break;

				if(bsdf.ContinuationProb() == 0)
					break;

				// next event estimation
				if(!bsdf.IsDelta() && pathLength + 1 >= mMinPathLength && lightCount > 0)
				{
					int lightID = int(mRng.GetFloat() * lightCount);
					const AbstractLight *light = mScene.GetLightPtr(lightID);

					Dir directionToLight;
					float distance, directPdfW;
					Rgb radiance = light->Illuminate(mScene.mSceneSphere, hitPoint,
						mRng.GetVec2f(), directionToLight, distance, directPdfW);

					if(!radiance.isBlackOrNegative())
					{
						float bsdfPdfW, cosThetaOut;
						const Rgb factor = bsdf.Evaluate(directionToLight, cosThetaOut, &bsdfPdfW);

						if(!factor.isBlackOrNegative())
						{
							float weight = 1.f;
							if(!light->IsDelta())
							{
								const float contProb = bsdf.ContinuationProb();
								bsdfPdfW *= contProb;
								weight = Mis2(directPdfW * lightPickProb, bsdfPdfW);
							}

							Rgb contrib = (weight * cosThetaOut / (lightPickProb * directPdfW)) *
								(radiance * factor);

							if(!mScene.Occluded(hitPoint, directionToLight, distance, mBoundaryStack))
							{
								color += pathWeight * contrib;
							}
						}
					}
				}

				// continue random walk
				{
					Dir rndTriplet = mRng.GetVec3f();
					float pdf, cosThetaOut;
					uint  sampledEvent;

					Rgb factor = bsdf.Sample(rndTriplet, ray.direction,
						pdf, cosThetaOut, &sampledEvent);

					if(factor.isBlackOrNegative())
						break;

					// Russian roulette
					const float contProb = bsdf.ContinuationProb();

					lastSpecular = (sampledEvent & BSDF::kSpecular) != 0;
					lastPdfW     = pdf * contProb;

					if(contProb < 1.f)
					{
						if(mRng.GetFloat() > contProb)
						{
							break;
						}
						pdf *= contProb;
					}

					pathWeight *= factor * (cosThetaOut / pdf);

					if ((sampledEvent & BSDF::kRefract) != 0)
						mScene.UpdateBoundaryStackOnRefract(isect, mBoundaryStack);

					ray.origin  = hitPoint;
					isect.mDist = 1e36f;
				}
			}
			mFramebuffer.AddColor(sample, color);
		}

		mIterations++;
	}

private:

	// MIS power (1 for balance heuristic)
	float Mis(float aPdf) const
	{
		return aPdf;
	}

	// MIS weight for 2 PDFs
	float Mis2(
		float aSamplePdf,
		float aOtherPdf) const
	{
		return Mis(aSamplePdf) / (Mis(aSamplePdf) + Mis(aOtherPdf));
	}

private:

	Rng mRng;
	BoundaryStack                   mBoundaryStack;
};

#endif //__PATHTRACER_HXX__