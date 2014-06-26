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

#ifndef __VOLPATHTRACER_HXX__
#define __VOLPATHTRACER_HXX__

#include <vector>
#include <cmath>

#include "Path\Bsdf.hxx"
#include "Renderers\Renderer.hxx"

class VolPathTracer : public AbstractRenderer
{
public:
	enum Version
    {
        kDirect,
		kLight,
		kMIS,
		kSpecOnly
    };

    VolPathTracer(
        const Scene& aScene,
        int aSeed = 1234,
		Version aVersion = kDirect
    ) :
	AbstractRenderer(aScene), mRng(aSeed), mVersion(aVersion)
    { }

    virtual void RunIteration(int aIteration)
    {
        // We sample lights uniformly
        const int   lightCount    = mScene.GetLightCount();
        const float lightPickProb = 1.f / lightCount;

        const int resX = int(mScene.mCamera.mResolution.get(0));
        const int resY = int(mScene.mCamera.mResolution.get(1));

        for (int pixID = 0; pixID < resX * resY; pixID++)
        {
            const int x = pixID % resX;
            const int y = pixID / resX;

			// Generate pixel sample
            const Vec2f sample = Vec2f(float(x), float(y)) + mRng.GetVec2f();

			// Create ray through the sample
            Ray ray = mScene.mCamera.GenerateRay(sample);
            Isect isect(1e36f);

			// Prepare few variables
            Rgb pathWeight(1.f);
            Rgb color(0.f);
            uint  pathLength   = 1;
            bool  lastSpecular = true;
			bool originInMedium  = false;
            float lastPdfW     = 1;
			int specularPath = 1;

			// Init the boundary stack with the global medium and add enclosing material and medium if present
			mScene.InitBoundaryStack(mBoundaryStack);
			if (mScene.mCamera.mMatID != -1 && mScene.mCamera.mMedID != -1) mScene.AddToBoundaryStack(mScene.mCamera.mMatID, mScene.mCamera.mMedID, mBoundaryStack);

			for(;; ++pathLength)
			{
				// Trace ray
				mVolumeSegments.clear();
				if (!mScene.Intersect(ray, originInMedium ? AbstractMedium::kOriginInMedium : 0, mRng, isect, mBoundaryStack, mVolumeSegments))
				{					
					// In attenuating media the ray can never travel to infinity
					//UPBP_ASSERT(mScene.GetGlobalMediumPtr()->GetAttenuationCoef(ray.origin).isBlackOrNegative());
					
					// We cannot end yet
					if (pathLength < mMinPathLength) 
						break;					
									
					// Get background light					
					const BackgroundLight* background = mScene.GetBackground();					

					// At the moment we do not support background illumination of scenes with emissive global medium
					if (background && !mScene.GetGlobalMediumPtr()->GetEmissionCoef(ray.origin).isBlackOrNegative())
						break;
					
					// Compute emission from intersected media (if any)
					if (!mVolumeSegments.empty() && (!background || mVersion != kLight || pathLength <= 1 || lastSpecular))					
						color += pathWeight * VolumeSegment::AccumulateAttenuatedEmissionWithPdf(mVolumeSegments);
					
					if (!background) 
						break;

					// In attenuating media the ray can never travel to infinity
					if (mScene.GetGlobalMediumPtr()->HasAttenuation())
						break;

					// Stop if we are in the light sampling mode and could have sampled this light last time in the next event estimation
					if (mVersion == kLight && pathLength > 1 && !lastSpecular) 
						break;

					// Attenuate by intersected media (if any)
					float raySamplePdf(1.0f);
					if (!mVolumeSegments.empty())
					{
						// PDF
						raySamplePdf = VolumeSegment::AccumulatePdf(mVolumeSegments);
						assert(raySamplePdf > 0);

						// Attenuation
						pathWeight *= VolumeSegment::AccumulateAttenuationWithoutPdf(mVolumeSegments) / raySamplePdf;					
					}				

					if (pathWeight.isBlackOrNegative()) 
						break;

					// Compute contribution
					
					// For background we cheat with the A/W suffixes,
					// and GetRadiance actually returns W instead of A
					float directPdfW;
					Rgb contrib = background->GetRadiance(mScene.mSceneSphere,
						ray.direction, Pos(0), &directPdfW);
					
					if (contrib.isBlackOrNegative()) 
						break;

					// Compute MIS weight (if in MIS mode and we could have sampled this light last time in the next event estimation)
					float misWeight = 1.f;
					if (mVersion == kMIS && pathLength > 1 && !lastSpecular)
					{						
						misWeight = Mis2(lastPdfW * raySamplePdf, directPdfW * lightPickProb);
					}

					// Add attenuated contribution
					color += pathWeight * misWeight * contrib;

					// We have left the scene
					break;
				}

				UPBP_ASSERT(isect.IsValid());
				
				// Attenuate by intersected media (if any)
				float raySamplePdf(1.0f);
				if (!mVolumeSegments.empty())
				{
					// Emission 
					// If in light sampling mode be careful not to create path which hits a light, adds emission on the path segment hitting it, but
					// not its emitted radiance. Such path cannot be created by direct path tracing.
					if (isect.mLightID < 0 || mVersion != kLight || pathLength <= 1 || lastSpecular)
						color += pathWeight * VolumeSegment::AccumulateAttenuatedEmissionWithPdf(mVolumeSegments);
					
					// PDF
					raySamplePdf = VolumeSegment::AccumulatePdf(mVolumeSegments);
					assert(raySamplePdf > 0);
					
					// Attenuation
					pathWeight *= VolumeSegment::AccumulateAttenuationWithoutPdf(mVolumeSegments) / raySamplePdf;					
				}				

				if (pathWeight.isBlackOrNegative()) 
					break;			
							
				// Prepare scattering function at the hitpoint (BSDF/phase depending on whether the hitpoint is at surface or in media, the isect knows)
				BSDF bsdf(ray, isect, mScene, BSDF::kFromCamera, mScene.RelativeIOR(isect, mBoundaryStack));
				
				if (!bsdf.IsValid()) // e.g. hitting surface too parallel with tangent plane
					break;

				if (bsdf.IsInMedium() && mVersion == kSpecOnly)
					break;

				// Compute hitpoint
				Pos hitPoint = ray.origin + ray.direction * isect.mDist;

				// Directly hit some light
				if (isect.mLightID >= 0)
				{
					// We cannot end yet
					if (pathLength < mMinPathLength) 
						break;

					// Stop if we are in the light sampling mode and could have sampled this light last time in the next event estimation
					if (mVersion == kLight && pathLength > 1 && !lastSpecular) 
						break;

					// Get hit light
					const AbstractLight *light = mScene.GetLightPtr(isect.mLightID);
					UPBP_ASSERT(light);

					// Compute its contribution
					float directIllumPdfA;
					Rgb contrib = light->GetRadiance(mScene.mSceneSphere,
						ray.direction, hitPoint, &directIllumPdfA);
					
					if(contrib.isBlackOrNegative()) 
						break;

					// Compute MIS weight (if in MIS mode and we could have sampled this light last time in the next event estimation)
					float misWeight = 1.f;
					if (mVersion == kMIS && pathLength > 1 && !lastSpecular)
					{						
						const float directIllumPdfW = PdfAtoW(directIllumPdfA, isect.mDist,
							bsdf.CosThetaFix());
						misWeight = Mis2(lastPdfW * raySamplePdf, directIllumPdfW * lightPickProb);
					}

					// Add attenuated contribution
					color += pathWeight * misWeight * contrib;
					
					// Lights do not reflect
					break;
				}					

				// Break if already at maximum path length
				if (pathLength >= mMaxPathLength) 
					break;

				// Get continuation probability
				const float contProb = bsdf.ContinuationProb();				

				// Get current medium
				int mediumID = mBoundaryStack.Top().mMediumId;
				AbstractMedium* medium = mScene.GetMediumPtr(mediumID);
				assert(medium);	

				// Next event estimation (if not in direct mode, not on a delta material, path is not too short for end and there are lights to sample)
				if (mVersion != kDirect && mVersion != kSpecOnly && !bsdf.IsDelta() && pathLength + 1 >= mMinPathLength && lightCount > 0)
				{					
					// Pick light
					int lightID = int(mRng.GetFloat() * lightCount);
					const AbstractLight *light = mScene.GetLightPtr(lightID);
					UPBP_ASSERT(light);

					// Light in infinity in attenuating homogeneous global medium is always reduced to zero, while emission along infinite ray is infinite
					if (light->IsFinite() || (mScene.GetGlobalMediumPtr()->GetAttenuationCoef(hitPoint).isBlackOrNegative() && mScene.GetGlobalMediumPtr()->GetEmissionCoef(hitPoint).isBlackOrNegative()))
					{
						// Sample light
						Dir directionToLight;
						float distanceToLight, directIllumPdfW;
						Rgb radiance = light->Illuminate(mScene.mSceneSphere, hitPoint,
							mRng.GetVec2f(), directionToLight, distanceToLight, directIllumPdfW);

						if (!radiance.isBlackOrNegative())
						{
							// Compute attenuation from scattering function
							float scatterPdfW, cosThetaOut;
							const Rgb scatterFactor = bsdf.Evaluate(directionToLight, cosThetaOut, &scatterPdfW);

							if (!scatterFactor.isBlackOrNegative())
							{
								// Test occlusion
								mVolumeSegments.clear();
								if (!mScene.Occluded(hitPoint, directionToLight, distanceToLight, mBoundaryStack, isect.IsInMedium() ? AbstractMedium::kOriginInMedium : 0, mVolumeSegments))
								{							
									// Get attenuation from intersected media (if any)
									float nextRaySamplePdf(1.0f);
									Rgb nextAttenuation(1.0f); 
									Rgb nextEmission(0.0f);
									if (!mVolumeSegments.empty())
									{
										// Emission (without PDF!)
										nextEmission = VolumeSegment::AccumulateAttenuatedEmissionWithoutPdf(mVolumeSegments);

										// PDF
										nextRaySamplePdf = VolumeSegment::AccumulatePdf(mVolumeSegments);
										UPBP_ASSERT(nextRaySamplePdf > 0);

										// Attenuation (without PDF!)
										nextAttenuation = VolumeSegment::AccumulateAttenuationWithoutPdf(mVolumeSegments);
									}

									// Compute MIS weight (if in MIS mode and it is possible to hit the light directly)
									float misWeight = 1.f;
									if (mVersion != kLight && !light->IsDelta())
									{								
										scatterPdfW *= contProb * nextRaySamplePdf; 
										misWeight = Mis2(directIllumPdfW * lightPickProb, scatterPdfW);
									}

									// Compute contribution
									Rgb contrib = (radiance * nextAttenuation + nextEmission) * scatterFactor * cosThetaOut / (lightPickProb * directIllumPdfW);							

									// Add attenuated contribution
									color += pathWeight * misWeight * contrib;							
								}
							}
						}
					}
				}

				// Continue random walk

				if (contProb == 0) 
					break;								

				// Scattering function sampling
				Dir rndTriplet = mRng.GetVec3f();
				float scatterPdf, cosThetaOut;
				uint  sampledEvent;

				Rgb scatterFactor = bsdf.Sample(rndTriplet, ray.direction,
					scatterPdf, cosThetaOut, &sampledEvent);

				if (scatterFactor.isBlackOrNegative()) 
					break;
				assert(scatterPdf > 0);

				// Russian roulette
				if (contProb < 1.f && mRng.GetFloat() > contProb) 
					break;

				scatterPdf *= contProb;
				lastPdfW = scatterPdf;

				pathWeight *= scatterFactor * (cosThetaOut / scatterPdf);
				
				if (pathWeight.isBlackOrNegative()) 
					break;

				// Update state according to the scattering function type
				if (isect.IsOnSurface()) 
				{					
					lastSpecular = (sampledEvent & BSDF::kSpecular) != 0;
					originInMedium = false;

					if (!lastSpecular && mVersion == kSpecOnly)
						break;

					// Switch medium on refraction
					if ((sampledEvent & BSDF::kRefract) != 0)
						mScene.UpdateBoundaryStackOnRefract(isect, mBoundaryStack);
				}
				else 
				{
					assert(medium == mScene.GetMediumPtr(isect.mMedID));

					lastSpecular = false;
					originInMedium = true;					
				}								
				specularPath &= lastSpecular ? 1 : 0;
				// Update ray
				ray.origin = hitPoint;
				isect.mDist = 1e36f;
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
    Rng                             mRng;
	Version                         mVersion;
	VolumeSegments                  mVolumeSegments;
	BoundaryStack                   mBoundaryStack;
};

#endif //__VOLPATHTRACER_HXX__
