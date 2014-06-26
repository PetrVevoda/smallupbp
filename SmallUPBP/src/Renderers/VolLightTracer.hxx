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

#ifndef __VOLLIGHTTRACER_HXX__
#define __VOLLIGHTTRACER_HXX__

#include <vector>
#include <cmath>

#include "..\Beams\PhBeams.hxx"
#include "..\Bre\Bre.hxx"
#include "..\Misc\Timer.hxx"
#include "..\Path\VltPathVertex.hxx"
#include "Renderer.hxx"

class VolLightTracer : public AbstractRenderer
{
    // The sole point of this structure is to make carrying around the ray baggage easier.
    struct SubPathState
    {
        Pos   mOrigin;             // Path origin
        Dir   mDirection;          // Where to go next
        Rgb   mThroughput;         // Path throughput
        uint  mPathLength    : 30; // Number of path segments, including this
        uint  mIsFiniteLight :  1; // Just generate by finite light
        uint  mSpecularPath  :  1; // All scattering events so far were specular

		BoundaryStack mBoundaryStack; // Boundary stack
    };

public:

	enum Version
    {
        kLightTracer  = 0x0001,
		kPointPoint3D = 0x0010,
		kPointPoint   = kPointPoint3D,
		kPointBeam2D  = 0x0100,
		kPointBeam3D  = 0x0200,
		kPointBeam    = kPointBeam2D|kPointBeam3D,
		kBeamBeam1D   = 0x1000,
		kBeamBeam2D   = 0x2000,
		kBeamBeam3D   = 0x4000,
		kBeamBeam     = kBeamBeam1D|kBeamBeam2D|kBeamBeam3D,
		kPhotons      = kPointPoint|kPointBeam|kBeamBeam
    };

	VolLightTracer(
		const Scene&            aScene,
		const int               aSeed,
		const Version           aVersion,
		const float			    aPB2DRadiusInitial,
		const float			    aPB2DRadiusAlpha,
		const RadiusCalculation aPB2DRadiusCalculation,		
		const int				aPB2DRadiusKNN,
		const BeamType	        aPB2DBeamType,
		const float			    aBB1DRadiusInitial,
		const float			    aBB1DRadiusAlpha,
		const RadiusCalculation aBB1DRadiusCalculation,		
		const int  			    aBB1DRadiusKNN,
		const BeamType		    aBB1DBeamType,
		const float             aBB1DUsedLightSubPathCount,
		const float             aRefPathCountPerIter,
		const bool              aVerbose = true
		) 
		: AbstractRenderer(aScene)
		, mRng(aSeed)
		, mVersion(aVersion)
		, mGlobalMediumID(aScene.mGlobalMediumID)
		, mEmbreeBre(aScene)
		, mPhotonBeams(aScene)
		, mVerbose(aVerbose)
		, mPB2DRadiusInitial(aPB2DRadiusInitial)
		, mPB2DRadiusAlpha(aPB2DRadiusAlpha)
		, mPB2DRadiusCalculation(aPB2DRadiusCalculation)		
		, mPB2DRadiusKNN(aPB2DRadiusKNN)
		, mPB2DBeamType(aPB2DBeamType)
		, mBB1DRadiusInitial(aBB1DRadiusInitial)
		, mBB1DRadiusAlpha(aBB1DRadiusAlpha)
		, mBB1DRadiusCalculation(aBB1DRadiusCalculation)	
		, mBB1DRadiusKNN(aBB1DRadiusKNN)
		, mBB1DBeamType(aBB1DBeamType)
		, mBB1DUsedLightSubPathCount(aBB1DUsedLightSubPathCount)
		, mRefPathCountPerIter(aRefPathCountPerIter)
	{
		if (mPB2DRadiusInitial < 0)
			mPB2DRadiusInitial = -mPB2DRadiusInitial * mScene.mSceneSphere.mSceneRadius;
		UPBP_ASSERT(mPB2DRadiusInitial > 0);

		if (mBB1DRadiusInitial < 0)
			mBB1DRadiusInitial = -mBB1DRadiusInitial * mScene.mSceneSphere.mSceneRadius;
		UPBP_ASSERT(mBB1DRadiusInitial > 0);

		mPhotonBeams.mSeed = aSeed;
	}

    virtual void RunIteration(int aIteration)
    {				
		if(mVerbose)
			std::cout << " + tracing light sub-paths..." << std::endl;
		
		mTimer.Start();

		// While we have the same number of pixels (camera paths)
        // and light paths, we do keep them separate for clarity reasons
        const int resX = int(mScene.mCamera.mResolution.get(0));
        const int resY = int(mScene.mCamera.mResolution.get(1));
		
		const int pathCount = resX * resY;
        mScreenPixelCount = float(resX * resY);
        mLightSubPathCount   = float(pathCount);
		
		if (mBB1DUsedLightSubPathCount < 0)
			mBB1DUsedLightSubPathCount = -mBB1DUsedLightSubPathCount * pathCount;

		// Radius reduction (1st iteration has aIteration == 0, thus offset)
		// PB2D
		float radiusPB2D = mPB2DRadiusInitial * std::pow(1 + aIteration * mLightSubPathCount / mRefPathCountPerIter, (mPB2DRadiusAlpha - 1) * 0.5f);		
		radiusPB2D = std::max(radiusPB2D, 1e-7f); // Purely for numeric stability
		const float radiusPB2DSqr = Utils::sqr(radiusPB2D);
		// BB1D
		float radiusBB1D = mBB1DRadiusInitial * std::pow(1 + aIteration * mBB1DUsedLightSubPathCount / mRefPathCountPerIter, mBB1DRadiusAlpha - 1);		
		radiusBB1D = std::max(radiusBB1D, 1e-7f); // Purely for numeric stability

        // Clear path ends
        mPathEnds.resize(pathCount);
        memset(&mPathEnds[0], 0, mPathEnds.size() * sizeof(int));

        // Remove all light vertices and reserve space for some	
		mLightVertices.clear();
		mPhotonBeamsArray.clear();
		if (mVersion & kPointBeam)
		{
			mLightVertices.reserve(pathCount);
		}
		else
		{
			mPhotonBeamsArray.reserve(pathCount * 2);
		}

        //////////////////////////////////////////////////////////////////////////
        // Generate light paths
        //////////////////////////////////////////////////////////////////////////

		if (mScene.GetLightCount() > 0 && mMaxPathLength > 1)
			for (int pathIdx = 0; pathIdx < pathCount; pathIdx++)
        {								
			// Generate light path origin and direction
			SubPathState lightState;
            GenerateLightSample(lightState);

			// In attenuating media the ray can never travel from infinity
			if (!lightState.mIsFiniteLight && mScene.GetGlobalMediumPtr()->HasAttenuation())
				continue;

			// We assume that the light is on surface
			bool originInMedium = false;

            //////////////////////////////////////////////////////////////////////////
            // Trace light path
            for (;; ++lightState.mPathLength)
            {
				// Prepare ray
                Ray ray(lightState.mOrigin, lightState.mDirection);
                Isect isect(1e36f);

				// Trace ray
				mVolumeSegments.clear();
				mLiteVolumeSegments.clear();
				bool intersected = mScene.Intersect(ray, originInMedium ? AbstractMedium::kOriginInMedium : 0, mRng, isect, lightState.mBoundaryStack, mVolumeSegments, mLiteVolumeSegments);

				// Store beam if required
				if ((mVersion & kBeamBeam) && pathIdx < mBB1DUsedLightSubPathCount)
				{
					AddBeams(ray, lightState.mThroughput);
				}

				if (!intersected)
					break;

				UPBP_ASSERT(isect.IsValid());

				// Same throughput before update by volume propagation
				const Rgb prevThroughput = lightState.mThroughput;
				
				// Attenuate by intersected media (if any)
				float raySamplePdf(1.0f);
				if (!mVolumeSegments.empty())
				{					
					// PDF
					raySamplePdf = VolumeSegment::AccumulatePdf(mVolumeSegments);
					UPBP_ASSERT(raySamplePdf > 0);
					
					// Attenuation
					lightState.mThroughput *= VolumeSegment::AccumulateAttenuationWithoutPdf(mVolumeSegments) / raySamplePdf;					
				}

				if (lightState.mThroughput.isBlackOrNegative()) 
					break;				

				// Prepare scattering function at the hitpoint (BSDF/phase depending on whether the hitpoint is at surface or in media, the isect knows)
				BSDF bsdf(ray, isect, mScene, BSDF::kFromLight, mScene.RelativeIOR(isect, lightState.mBoundaryStack));
            
				if (!bsdf.IsValid()) // e.g. hitting surface too parallel with tangent plane
					break;

				// Compute hitpoint
                const Pos hitPoint = ray.origin + ray.direction * isect.mDist;

				originInMedium = isect.IsInMedium();

                // Store vertex, unless scattering function is purely specular, which prevents
                // vertex connections and merging (not used by pure light tracing)
				if (mVersion & kPointBeam )
				{
					if (!bsdf.IsDelta())
					{
						VltLightVertex lightVertex;
						lightVertex.mHitpoint = hitPoint;
						lightVertex.mIncEdgeLength = isect.mDist;
						lightVertex.mThroughput = lightState.mThroughput;
						lightVertex.mPrevThroughput = prevThroughput;
						lightVertex.mPathLength = lightState.mPathLength;
						lightVertex.mIsInMedium = isect.IsInMedium();
						lightVertex.mBSDF = bsdf;

						mLightVertices.push_back(lightVertex);
					}
				}
				
                // Connect to camera, unless scattering function is purely specular
                if ( (mVersion & kLightTracer) && (!bsdf.IsDelta()) )
                {
                    if (lightState.mPathLength + 1 >= mMinPathLength)
                        ConnectToCamera(lightState, hitPoint, bsdf, isect);
                }

                // Terminate if the path would become too long after scattering
                if (lightState.mPathLength + 2 > mMaxPathLength) 
					break;				

                // Continue random walk
                if (!SampleScattering(bsdf, hitPoint, lightState, isect)) 
					break;				
            }

            mPathEnds[pathIdx] = (int)mLightVertices.size(); // (not used by pure light tracing)
        }

		mTimer.Stop();
		if(mVerbose)
			std::cout << "    - light sub-path tracing done in " << mTimer.GetLastElapsedTime() << " sec. " << std::endl;
		
		//////////////////////////////////////////////////////////////////////////
		// Generate primary rays & accumulate "photon" contributions.
		//////////////////////////////////////////////////////////////////////////

		if (((mVersion & kBeamBeam) && !mPhotonBeamsArray.empty()) || ((mVersion & kPointBeam) && !mLightVertices.empty()))
		{
			CalculatePhotonContributions(resX, resY);
		}

        mIterations++;
    }

private:

    //////////////////////////////////////////////////////////////////////////
    // Photon mapping methods
    //////////////////////////////////////////////////////////////////////////

	void CalculatePhotonContributions(const int resX, const int resY)
	{
		UPBP_ASSERT( mVersion & kPhotons );
		
		if( mMaxPathLength > 1 )
		{
			if( mVersion & kPointBeam )
			{
				mEmbreeBre.build(&mLightVertices[0], (int)mLightVertices.size(), mPB2DRadiusCalculation,mPB2DRadiusInitial,mPB2DRadiusKNN,mVerbose);
			}

			if( mVersion & kBeamBeam )
			{
				mPhotonBeams.build(mPhotonBeamsArray, mBB1DRadiusCalculation, mBB1DRadiusInitial, mBB1DRadiusKNN, mVerbose);
			}
		}
	
		if(mVerbose)
			std::cout << " + tracing primary rays..." << std::endl;
		mTimer.Start();

		for (int pixID = 0; pixID < resX * resY; pixID++)
		{
			const int x = pixID % resX;
			const int y = pixID / resX;

			// Generate pixel sample
			const Vec2f sample = Vec2f(float(x), float(y)) + mRng.GetVec2f();

			// Create ray through the sample
			Ray ray = mScene.mCamera.GenerateRay(sample);
			Isect isect(1e36f);

			// Init the boundary stack with the global medium and add enclosing material and medium if present
			mScene.InitBoundaryStack(mBoundaryStack);
			if (mScene.mCamera.mMatID != -1 && mScene.mCamera.mMedID != -1) mScene.AddToBoundaryStack(mScene.mCamera.mMatID, mScene.mCamera.mMedID, mBoundaryStack);

			bool  originInMedium  = false;
			float rayLength;
			Rgb   surfaceRadiance(0.f), volumeRadiance(0.f);

			mVolumeSegments.clear();

			// Cast primary ray (do not scatter in media - pass all the way until we hit some object)
			bool hitSomething = mScene.Intersect(ray, isect, mBoundaryStack, mPB2DBeamType == SHORT_BEAM ? Scene::kSampleVolumeScattering : 0, 0, &mRng, &mVolumeSegments, NULL, NULL);

			const AbstractMedium* medium = (isect.mMedID >= 0) ? mScene.GetMediumPtr(isect.mMedID) : nullptr;

			if( hitSomething )
			{
				rayLength = isect.mDist;
				if (isect.mLightID >= 0)
				{
					// Directly hit some light
					const AbstractLight *light = mScene.GetLightPtr(isect.mLightID);
					UPBP_ASSERT(light);

					// Compute directly emitted radiance
					float directIllumPdfA;
					surfaceRadiance = light->GetRadiance(mScene.mSceneSphere,
						ray.direction, ray.target(isect.mDist), &directIllumPdfA);

					// Attenuate surface radiance by media extinction
					if(medium)
					{
						surfaceRadiance *= medium->EvalAttenuation(ray, 0, isect.mDist);
					}

					UPBP_ASSERT( !surfaceRadiance.isNanInfNeg() );
				}
			}
			else
			{
				// Didn't hit anything - we ignore background light for now.
				rayLength = INFINITY;
			}

			if ( (mMaxPathLength > 1) && !mVolumeSegments.empty() )
			{
				UPBP_ASSERT(medium);

				if( mVersion & kPointBeam )
				{
					volumeRadiance = mEmbreeBre.evalBre(mPB2DBeamType, ray, mVolumeSegments) / mLightSubPathCount;
				}

				if( mVersion & kBeamBeam )
				{
					GridStats gridStats;
					volumeRadiance = mPhotonBeams.evalBeamBeamEstimate(mBB1DBeamType, ray, mVolumeSegments, BB1D, 0, NULL, &gridStats) / mBB1DUsedLightSubPathCount;

					mBeamDensity.Accumulate(pixID, gridStats);
				}

				UPBP_ASSERT( !volumeRadiance.isNanInfNeg() );
			}

			mFramebuffer.AddColor(sample, surfaceRadiance + volumeRadiance);
		}

		mTimer.Stop();
		if(mVerbose)
			std::cout << std::setprecision(3) << "   - primary ray tracing done in " << mTimer.GetLastElapsedTime() << " sec. " << std::endl;

		mCameraTracingTime += mTimer.GetLastElapsedTime();

		if( mMaxPathLength > 1 )
		{
			if( mVersion & kPointBeam )
			{
				mEmbreeBre.destroy();
			}

			if( mVersion & kBeamBeam )
			{
				mPhotonBeams.destroy();
			}
		}
	}

    //////////////////////////////////////////////////////////////////////////
    // Light tracing methods
    //////////////////////////////////////////////////////////////////////////

    // Samples light emission
    void GenerateLightSample(SubPathState &oLightState)
    {
        // Sample lights uniformly
        
		const int   lightCount     = mScene.GetLightCount();
        const float lightPickProb  = 1.f / lightCount;
        
		const int   lightID        = int(mRng.GetFloat() * lightCount);
        const AbstractLight *light = mScene.GetLightPtr(lightID);
		UPBP_ASSERT(light);

		// Generate light path origin and direction
		
		const Vec2f rndDirSamples = mRng.GetVec2f();
        const Vec2f rndPosSamples = mRng.GetVec2f();
        float emissionPdfW, directPdfW, cosLight;
        
		oLightState.mThroughput = light->Emit(mScene.mSceneSphere, rndDirSamples, rndPosSamples,
            oLightState.mOrigin, oLightState.mDirection,
            emissionPdfW, &directPdfW, &cosLight);

		// Complete light path state initialization

        emissionPdfW *= lightPickProb;
        directPdfW   *= lightPickProb;  // (not used by pure light tracing)
       
		oLightState.mThroughput    /= emissionPdfW;
        oLightState.mPathLength    = 1;
        oLightState.mIsFiniteLight = light->IsFinite() ? 1 : 0;

		// Init the boundary stack with the global medium and add enclosing material and medium if present
		mScene.InitBoundaryStack(oLightState.mBoundaryStack);
		if (light->mMatID != -1 && light->mMedID != -1) mScene.AddToBoundaryStack(light->mMatID, light->mMedID, oLightState.mBoundaryStack);
    }

    // Computes contribution of light sample to camera by splatting is onto the
    // framebuffer. Multiplies by throughput (obviously, as nothing is returned).
    void ConnectToCamera(
        const SubPathState               &aLightState,
        const Pos                        &aHitpoint,
        const BSDF                       &aLightBSDF,
		const Isect                      &isect)
    {
		// Get camera and direction to it
        const Camera &camera  = mScene.mCamera;
		Dir directionToCamera = camera.mOrigin - aHitpoint;

        // Check point is in front of camera
		if (dot(camera.mDirection, -directionToCamera) <= 0.f)
            return;

        // Check it projects to the screen (and where)
        const Vec2f imagePos = camera.WorldToRaster(aHitpoint);        
		if (!camera.CheckRaster(imagePos))
            return;

        // Compute distance and normalize direction to camera
        const float distEye2 = directionToCamera.square();
        const float distance = std::sqrt(distEye2);
        directionToCamera   /= distance;

        // Get the scattering function factor
        float cosToCamera, sfDirPdfW, sfRevPdfW;
        Rgb sfFactor = aLightBSDF.Evaluate(directionToCamera, cosToCamera, &sfDirPdfW, &sfRevPdfW);
        
		if (sfFactor.isBlackOrNegative())
            return;

        sfRevPdfW *= aLightBSDF.ContinuationProb();

        // Compute PDF conversion factor from image plane area to surface area
		const float cosAtCamera = dot(camera.mDirection, -directionToCamera);
        const float imagePointToCameraDist = camera.mImagePlaneDist / cosAtCamera;
        const float imageToSolidAngleFactor = Utils::sqr(imagePointToCameraDist) / cosAtCamera;
        const float imageToSurfaceFactor = imageToSolidAngleFactor * std::abs(cosToCamera) / Utils::sqr(distance);

        // We put the virtual image plane at such a distance from the camera origin
        // that the pixel area is one and thus the image plane sampling PDF is 1.
        // The area PDF of aHitpoint as sampled from the camera is then equal to
        // the conversion factor from image plane area density to surface area density
        const float cameraPdfA = imageToSurfaceFactor;

        const float surfaceToImageFactor = 1.f / imageToSurfaceFactor;

		mVolumeSegments.clear();
		if (!mScene.Occluded(aHitpoint, directionToCamera, distance, aLightState.mBoundaryStack, isect.IsOnSurface() ?  0 : AbstractMedium::kOriginInMedium, mVolumeSegments))
        {			
			// Get attenuation from intersected media (if any)
			Rgb mediaAttenuation = mVolumeSegments.empty() ? Rgb(1.0) : VolumeSegment::AccumulateAttenuationWithoutPdf(mVolumeSegments);

			// We divide the contribution by surfaceToImageFactor to convert the (already
			// divided) PDF from surface area to image plane area, w.r.t. which the
			// pixel integral is actually defined. We also divide by the number of samples
			// this technique makes, which is equal to the number of light sub-paths
			Rgb contrib = aLightState.mThroughput * sfFactor * mediaAttenuation / (mLightSubPathCount * surfaceToImageFactor);
			
			if (contrib.isBlackOrNegative())
				return;

            mFramebuffer.AddColor(imagePos, contrib);
        }
    }

    // Samples a scattering direction camera/light sample according to scattering function.
    // Returns false for termination
    bool SampleScattering(
		const BSDF      &aBSDF,
        const Pos       &aHitPoint,
        SubPathState    &aoState,
		const Isect     &isect)
    {
        // Sample scattering function		
		
        Dir   rndTriplet  = mRng.GetVec3f(); // x,y for direction, z for component. No rescaling happens
        float sfDirPdfW, cosThetaOut;
        uint  sampledEvent;

        Rgb sfFactor = aBSDF.Sample(rndTriplet, aoState.mDirection,
            sfDirPdfW, cosThetaOut, &sampledEvent);
        
		if (sfFactor.isBlackOrNegative())
            return false;

		bool specular = (sampledEvent & BSDF::kSpecular) != 0;

        // If we sampled specular event, then the reverse probability
        // cannot be evaluated, but we know it is exactly the same as
        // forward probability, so just set it. If non-specular event happened,
        // we evaluate the PDF
        float sfRevPdfW = sfDirPdfW;
        if (!specular)
			sfRevPdfW = aBSDF.Pdf(aoState.mDirection, BSDF::kReverse);

        // Russian roulette
        const float contProb = aBSDF.ContinuationProb();
        if (contProb == 0 || (contProb < 1.0f && mRng.GetFloat() > contProb))
            return false;

        sfDirPdfW *= contProb;
        sfRevPdfW *= contProb;                

		// Update throughput
		aoState.mThroughput *= sfFactor * (cosThetaOut / sfDirPdfW);

		// Switch medium on refraction
		if ((sampledEvent & BSDF::kRefract) != 0)
			mScene.UpdateBoundaryStackOnRefract(isect, aoState.mBoundaryStack);

		// Update the rest of the light path state

		if (specular)
        {
            UPBP_ASSERT(sfDirPdfW == sfRevPdfW);
            aoState.mSpecularPath &= 1;
        }
        else
        {
            aoState.mSpecularPath &= 0;
        }		

		aoState.mOrigin = aHitPoint;

        return true;
    }

	// Adds beams to beams array
	void AddBeams(
		const Ray		&aRay,
		const Rgb		&aThroughput
		)
	{
		Rgb throughput = aThroughput;
		if (mBB1DBeamType == SHORT_BEAM)
		{
			for (VolumeSegments::const_iterator it = mVolumeSegments.cbegin(); it != mVolumeSegments.cend(); ++it)
			{
				UPBP_ASSERT(it->mMediumID >= 0);
				PhotonBeam beam;
				beam.mMedium = mScene.mMedia[it->mMediumID];
				if (beam.mMedium->HasScattering())
				{
					beam.mRay = Ray(aRay.origin + aRay.direction * it->mDistMin, aRay.direction);
					beam.mLength = it->mDistMax - it->mDistMin;
					beam.mFlags = SHORT_BEAM;
					beam.mThroughputAtOrigin = throughput;
					mPhotonBeamsArray.push_back(beam);
				}
				throughput *= it->mAttenuation / it->mRaySamplePdf;
			}
		}
		else // LONG_BEAM
		{
			UPBP_ASSERT(mBB1DBeamType == LONG_BEAM);
			for (LiteVolumeSegments::const_iterator it = mLiteVolumeSegments.cbegin(); it != mLiteVolumeSegments.cend(); ++it)
			{
				UPBP_ASSERT(it->mMediumID >= 0);
				PhotonBeam beam;
				beam.mMedium = mScene.mMedia[it->mMediumID];
				if (beam.mMedium->HasScattering())
				{
					beam.mRay = Ray(aRay.origin + aRay.direction * it->mDistMin, aRay.direction);
					beam.mLength = it->mDistMax - it->mDistMin;
					beam.mFlags = LONG_BEAM;
					beam.mThroughputAtOrigin = throughput;
					mPhotonBeamsArray.push_back(beam);
				}
				if (beam.mMedium->IsHomogeneous())
				{
					const HomogeneousMedium * medium = ((const HomogeneousMedium *)beam.mMedium);
					throughput *= medium->EvalAttenuation(it->mDistMax - it->mDistMin);
				}
				else
				{
					throughput *= beam.mMedium->EvalAttenuation(aRay, it->mDistMin, it->mDistMax);
				}
			}

			if (!mPhotonBeamsArray.empty() && mPhotonBeamsArray.back().mLength > mPhotonBeamsArray.back().mMedium->MaxBeamLength())
				mPhotonBeamsArray.back().mLength = mPhotonBeamsArray.back().mMedium->MaxBeamLength();
		}
	}

private:
    float mScreenPixelCount;    // Number of pixels
    float mLightSubPathCount;   // Number of light sub-paths
	float mRefPathCountPerIter; // Reference number of paths per iteration

	typedef std::vector<VltLightVertex> LightVertexVector;
    LightVertexVector mLightVertices;       // Stored light vertices
	PhotonBeamsArray mPhotonBeamsArray;	    // Stores photon beams
	VolumeSegments mVolumeSegments;         // Path segments intersecting media (up to scattering point)
	LiteVolumeSegments mLiteVolumeSegments; // Lite path segments intersecting media (up to intersection with solid surface)

	EmbreeBre mEmbreeBre;
	PhotonBeamsEvaluator mPhotonBeams;

	int mGlobalMediumID;

    // For light path belonging to pixel index [x] it stores
    // where it's light vertices end (begin is at [x-1])
    std::vector<int>  mPathEnds;
    Rng               mRng;
	BoundaryStack     mBoundaryStack;
	Version           mVersion;
	bool              mVerbose;
	Timer             mTimer;
	
	// PB2D
	float             mPB2DRadiusInitial;     // Initial merging radius
	float             mPB2DRadiusAlpha;       // Radius reduction rate parameter
	RadiusCalculation mPB2DRadiusCalculation; // Type of photon radius calculation	
	int	              mPB2DRadiusKNN;	      // Value x means that x-th closest photon will be used for calculation of radius of the current photon
	BeamType          mPB2DBeamType;          // Short/long beam

	// BB1D
	float                mBB1DRadiusInitial;         // Initial merging radius
	float                mBB1DRadiusAlpha;           // Radius reduction rate parameter
	RadiusCalculation    mBB1DRadiusCalculation;     // Type of photon radius calculation
	int	                 mBB1DRadiusKNN;             // Value x means that x-th closest beam vertex will be used for calculation of cone radius at the current beam vertex
	BeamType             mBB1DBeamType;              // Short/long beam
	float                mBB1DUsedLightSubPathCount; // First mBB1DUsedLightSubPathCount out of mLightSubPathCount light paths will generate photon beams
};

#endif //__VOLLIGHTTRACER_HXX__
