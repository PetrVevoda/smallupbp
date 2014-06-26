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

#ifndef __SCENE_HXX__
#define __SCENE_HXX__

#pragma warning(disable: 4482)

#include <algorithm>
#include <vector>
#include <map>
#include <cmath>

#include "..\Misc\Rng.hxx"
#include "..\Misc\ObjReader.hxx"
#include "..\Path\BoundaryStack.hxx"
#include "Geometry.hxx"
#include "Camera.hxx"
#include "Lights.hxx"
#include "Medium.hxx"

class Scene
{
public:
	class SceneConfig
	{
	public:		
		enum Lights
		{			
			kLightCeilingAreaBig,
			kLightCeilingAreaSmall,
			kLightCeilingAreaSmallDistant,
			kLightCeilingPoint,
			kLightFacingAreaSmall,
			kLightFacingPoint,
			kLightSun, 
			kLightBackground,
			kLightNone,
			kLightsCount		
		};

		enum Geometry
		{
			kLeftWall,
			kRightWall,
			kFrontFacingFrontWall,
			kBackFacingFrontWall,
			kBackWall,
			kCeiling,
			kFloor,
			kLargeSphereMiddle,
			kSmallSphereLeft,
			kSmallSphereRight,
			kSmallSphereBottomLeft,
			kSmallSphereBottomRight,
			kSmallSphereTop,
			kVeryLargeSphere,
			kVeryLargeBox,
			kGeometryCount
		 };

		enum Materials
		{			
			kNoMaterial = -1,
			kWaterMaterial,
			kIce,
			kGlass,
			kDiffuseRed,
			kDiffuseGreen,
			kDiffuseBlue,
			kDiffuseWhite,
			kGlossyWhite,
			kMirror,
			kLight,
			kMaterialsCount
		};

		enum Media
		{
			kNoMedium = -1,
			kClear,
			kWaterMedium,
			kWhiteIsoScattering, // color in name is color of result
			kWhiteAnisoScattering,
			kWeakWhiteIsoScattering,
			kWeakYellowIsoScattering,
			kWeakWhiteAnisoScattering,
			kRedAbsorbing, 
			kYellowEmitting,
			kYellowGreenAbsorbingEmittingAndScattering,
			kBlueAbsorbingAndEmitting,
			kLightReddishMedium,
			kAbsorbingAnisoScattering,
			kMediaCount
		};

		struct Element
		{
		public:
			Element(Geometry aGeometry, Materials aMaterial, Media aMedium)
			{
				Setup(aGeometry, aMaterial, aMedium);
			}

			Element(Geometry aGeometry, Materials aMaterial)
			{
				Setup(aGeometry, aMaterial, Media::kNoMedium);
			}

			Element(Geometry aGeometry, Media aMedium)
			{
				Setup(aGeometry, Materials::kNoMaterial, aMedium);
			}

			Element(Geometry aGeometry)
			{
				Setup(aGeometry, Materials::kDiffuseWhite, Media::kNoMedium);
			}

			Geometry  mGeometry;
			Materials mMaterial;
			Media     mMedium;

		private:
			void Setup(Geometry aGeometry, Materials aMaterial, Media aMedium)
			{
				mGeometry = aGeometry;
				mMaterial = aMaterial;
				mMedium   = aMedium;
			}
		};

	public:
		SceneConfig(std::string aShortName, std::string aLongName, Lights aLight, Media aGlobalMedium)
		{
			Reset(aShortName, aLongName, aLight, aGlobalMedium);
		}

		void Reset(std::string aShortName, std::string aLongName, Lights aLight, Media aGlobalMedium)
		{
			mShortName = aShortName;
			mLongName = aLongName;
			mLight = aLight;
			mGlobalMedium = aGlobalMedium;
			mElements.clear();
		}

		void AddElement(Element element)
		{
			mElements.push_back(element);
		}

		void AddAllElements(std::vector<Element> aElements)
		{
			mElements.insert(mElements.end(), aElements.begin(), aElements.end());
		}

	public:
		std::string mShortName;
		std::string mLongName;		
		Lights mLight;
		Media mGlobalMedium;
		std::vector<Element> mElements;
	};

public:
    Scene() :
        mRealGeometry(NULL),
		mImaginaryGeometry(NULL),
        mBackground(NULL)
    {}

    ~Scene()
    {
        delete mRealGeometry;
		delete mImaginaryGeometry;

        for(size_t i=0; i<mLights.size(); i++)
            delete mLights[i];

		for(size_t i=0; i<mMedia.size(); i++)
            delete mMedia[i];
    }

	enum IntersectOptions
	{
		/// Act as if there were no participating media in the scene
		kIgnoreMediaAltogether = 0x0001,

		/// Sample scattering distance in the medium (if not set, always returns the nearest surface as the scattering point)
		kSampleVolumeScattering = 0x0002,

		kOcclusionTest = 0x0004
	};

	bool Intersect(
		const Ray          &aRay,
		Isect              &oResult,
		BoundaryStack      &oBoundaryStack) const
	{
		return Intersect(aRay, oResult, oBoundaryStack, kIgnoreMediaAltogether, 0, NULL, NULL, NULL, NULL);
	}

	bool Intersect(
		const Ray          &aRay,
		Isect              &oResult,
		BoundaryStack      &oBoundaryStack,
		LiteVolumeSegments &oVolumeSegmentsAll) const
	{
		return Intersect(aRay, oResult, oBoundaryStack, 0, 0, NULL, NULL, &oVolumeSegmentsAll, NULL);
	}

	bool Intersect(
		const Ray          &aRay,
		const uint         aRaySamplingFlags,
		Rng                &aRng,
		Isect              &oResult,
		BoundaryStack      &oBoundaryStack,
		VolumeSegments     &oVolumeSegmentsToIsect) const
	{
		return Intersect(aRay, oResult, oBoundaryStack, kSampleVolumeScattering, aRaySamplingFlags, &aRng, &oVolumeSegmentsToIsect, NULL, NULL);
	}

	bool Intersect(
		const Ray          &aRay,
		Rng                &aRng,
		const uint         aOptions,
		Isect              &oResult,
		BoundaryStack      &oBoundaryStack,
		VolumeSegments     &oVolumeSegmentsToIsect) const
	{
		return Intersect(aRay, oResult, oBoundaryStack, aOptions, 0, &aRng, &oVolumeSegmentsToIsect, NULL, NULL);
	}

	bool Intersect(
		const Ray          &aRay,
		const uint         aRaySamplingFlags,
		Rng                &aRng,
		Isect              &oResult,
		BoundaryStack      &oBoundaryStack,
		VolumeSegments     &oVolumeSegmentsToIsect,
		LiteVolumeSegments &oVolumeSegmentsAll) const
	{
		return Intersect(aRay, oResult, oBoundaryStack, kSampleVolumeScattering, aRaySamplingFlags, &aRng, &oVolumeSegmentsToIsect, &oVolumeSegmentsAll, NULL);
	}

	bool Intersect(
		const Ray          &aRay,		
		Isect              &oResult,
		BoundaryStack      &oBoundaryStack,
		const uint         aOptions,
		const uint         aRaySamplingFlags,
		Rng                *aRng,
		VolumeSegments     *oVolumeSegmentsToIsect,
		LiteVolumeSegments *oVolumeSegmentsAll,
		float              *oDistToReal) const
	{			
		bool ignoreMedia = (aOptions & kIgnoreMediaAltogether) != 0;
		bool sampleMedia = (aOptions & kSampleVolumeScattering) != 0;
		bool testOcclusion = (aOptions & kOcclusionTest) != 0;
		
		// We cannot sample media if they are ignored, if we know that the ray has already got through them or if we don't have the random number generator
		if (ignoreMedia || testOcclusion || !aRng)
			sampleMedia = false;

		Ray tempRay(aRay);
		Isect tempResult(oResult);
		float dist = 0;

		////////////////////////////////////////////////////////////////////////////////////
		// Real intersection
		// We can compute this separately only because we assume that no medium without a material
		// has greater priority than or equal to any of media with a material!
		// In other words, no medium without a material can be inside a medium with a material (it will be ignored).
		// We also assume that no boundary of a medium without material intersects any boundary of media with material.

		// Crossed boundaries with low priority are stored in list and their insertion in boundary stack
		// is postponed until media interaction is processed 
		Intersections hitsWithMaterial;
		
		UPBP_ASSERT(!oBoundaryStack.IsEmpty());
		int topPriority = oBoundaryStack.TopPriority();

		// Epsilon offset of the ray origin, zero in media
		const float firstEps = (aRaySamplingFlags & AbstractMedium::kOriginInMedium) ? 0 : EPS_RAY;

		// Maximum distance to intersection
		float missDist = tempResult.mDist;
		if (!testOcclusion) missDist = INFINITY;

		// Current t max. In case of occlusion test we subtract two epsilons, one at the beginning of the ray because of origin offset, 
		// one at the end in order not to accidentally hit the target surface
		if (testOcclusion) tempResult.mDist -= firstEps + EPS_RAY;
		else missDist = tempResult.mDist = INFINITY;			

		bool hit;
		bool notYetReal;

		BoundaryStack stackCopy(oBoundaryStack);

		float realEps = firstEps;
		do
		{
			notYetReal = false;
			
			// Try find intersection. We use origin epsilon offset for numerical stable intersection, but we immediately
			// restore real origin and distance afterwards for clarity
			
			tempRay.origin = tempRay.origin + tempRay.direction * realEps;
			hit = mRealGeometry->Intersect(tempRay, tempResult);
			tempRay.origin = tempRay.origin - tempRay.direction * realEps;

			if (hit)
			{
				tempResult.mDist += realEps;
				dist += tempResult.mDist;

				UPBP_ASSERT(tempResult.mMatID >= 0);

				int priority = GetMediumBoundaryPriority(tempResult.mMatID);

				StackElement top = stackCopy.Top();

				if (tempResult.mMedID < 0) tempResult.mMedID = top.mMediumId;

				// Check if it enters or leaves material with lower priority
				if (priority < topPriority && priority != THIN_WALL_PRIORITY)
				{
					StackElement element(tempResult.mMatID, tempResult.mMedID);
					if (tempResult.mEnter)
					{
						hitsWithMaterial.push_back(tempResult);
						stackCopy.Push(element, priority);
					}
					else
					{
						// Ignore hit, if it was not in a stack. This 'if' is why we need to update (a copy of) the stack.
						if (stackCopy.Pop(element, priority))
							hitsWithMaterial.push_back(tempResult);
					}

					notYetReal = true;
					// Prepare the ray for the next step
					realEps = EPS_RAY;
					tempRay.origin = tempRay.origin + tempRay.direction * tempResult.mDist;
					if (testOcclusion) tempResult.mDist = missDist - dist - realEps - EPS_RAY;
					else tempResult.mDist = INFINITY;		
				}					
			}

		} while (notYetReal && tempResult.mDist > 0);

		if (!hit || notYetReal)
		{
			hit = false;
			dist = missDist;
		}

		oResult = tempResult;
		oResult.mDist = dist;

		if (oDistToReal) *oDistToReal = dist;

		////////////////////////////////////////////////////////////////////////////////////
		// Media interaction

		bool scatteringOccured = false;

		// We need to deal with media
		if (!ignoreMedia && (sampleMedia || oVolumeSegmentsToIsect || oVolumeSegmentsAll))
		{			
			// Get lite volume segments

			// Store them in the output parameter if given
			LiteVolumeSegments _allSegments;
			LiteVolumeSegments* allSegments = oVolumeSegmentsAll;
			if (!oVolumeSegmentsAll) allSegments = &_allSegments;

			Intersections imaginaryIsects;

			// If we are not already behind any material and there is enough space before the first material intersection 
			// we can check intersection with the media without a material (the same assumptions as above apply here)
			if (!IsThisRealGeometry(oBoundaryStack.Top().mMaterialId) && dist > firstEps + EPS_RAY)
			{				
				tempRay.origin = aRay.origin + aRay.direction * firstEps;
				mImaginaryGeometry->IntersectAll(tempRay, dist - firstEps - EPS_RAY, imaginaryIsects);
				tempRay.origin = tempRay.origin - tempRay.direction * firstEps;

				stackCopy = BoundaryStack(oBoundaryStack);
				float distMin = 0;

				for (Intersections::iterator i = imaginaryIsects.begin(); i != imaginaryIsects.end(); ++i)
				{
					i->mDist += firstEps;
					int priority = GetMediumBoundaryPriority(i->mMatID);
					StackElement element(i->mMatID, i->mMedID);
					
					if (i->mEnter)
					{
						if (priority >= stackCopy.TopPriority())
						{
							VolumeSegmentLite segment;
							segment.mDistMin = distMin;
							segment.mDistMax = i->mDist;
							segment.mMediumID = stackCopy.Top().mMediumId;
							allSegments->push_back(segment);
							distMin = i->mDist;
						}

						stackCopy.Push(element, priority);
					}
					else
					{
						if (priority == stackCopy.TopPriority() && element == stackCopy.Top())
						{
							VolumeSegmentLite segment;
							segment.mDistMin = distMin;
							segment.mDistMax = i->mDist;
							segment.mMediumID = stackCopy.Top().mMediumId;
							allSegments->push_back(segment);
							distMin = i->mDist;
						}

						stackCopy.Pop(element, priority);
					}
				}

				if (distMin < dist )
				{
					VolumeSegmentLite segment;
					segment.mDistMin = distMin;
					segment.mDistMax = dist;
					segment.mMediumID = stackCopy.Top().mMediumId;
					allSegments->push_back(segment);
				}
			}
			else
			{
				// Otherwise there is only one segment
				VolumeSegmentLite segment;
				segment.mDistMin = 0;
				segment.mDistMax = dist;
				segment.mMediumID = oBoundaryStack.Top().mMediumId;
				allSegments->push_back(segment);
			}

			// Get full volume segments

			if (sampleMedia || oVolumeSegmentsToIsect)
			{
				// Store them in the output parameter if given
				VolumeSegments _segmentsToIsect;
				VolumeSegments* segmentsToIsect = oVolumeSegmentsToIsect;
				if (!oVolumeSegmentsToIsect) segmentsToIsect = &_segmentsToIsect;

				bool originInMedium = (aRaySamplingFlags & AbstractMedium::kOriginInMedium) != 0;
				bool endInMedium = (aRaySamplingFlags & AbstractMedium::kEndInMedium) != 0;

				for (LiteVolumeSegments::const_iterator i = allSegments->cbegin(); i != allSegments->cend(); ++i)
				{					
					// Get medium in current segment
					int currentMediumID = i->mMediumID;
					if (currentMediumID < 0) continue;
					AbstractMedium *currentMediumPtr = GetMediumPtr(currentMediumID);
					UPBP_ASSERT(currentMediumPtr);

					tempRay = Ray(aRay.origin + i->mDistMin * aRay.direction, aRay.direction);
					tempResult.mDist = i->mDistMax - i->mDistMin;

					uint raySamplingFlags = 0;
					if (originInMedium && i->mDistMin == 0) raySamplingFlags |= AbstractMedium::kOriginInMedium;
					if (endInMedium && i->mDistMax == dist) raySamplingFlags |= AbstractMedium::kEndInMedium;
					
					float raySamplePdf = 1.0f;
					float raySampleRevPdf = 1.0f;

					float distMax = i->mDistMax;
			
					if (currentMediumPtr->HasScattering())
					{
						if (sampleMedia)
						{
							// To avoid sampling zero direction
							float random = aRng->GetFloat();
							while (random == 1) random = aRng->GetFloat();
							
							// Sample ray within the bounds of the segment
							float distToNextSegment = tempResult.mDist;
							float distToMedium = currentMediumPtr->SampleRay(tempRay, distToNextSegment, random, &raySamplePdf, raySamplingFlags, &raySampleRevPdf);

							// Output medium hit point if sampled inside medium
							if (distToMedium < distToNextSegment)
							{								
								oResult.mDist = i->mDistMin + distToMedium;					
								oResult.mMatID = -1;
								oResult.mMedID = currentMediumID;
								oResult.mLightID = -1;
								oResult.mNormal = Dir(0.0f);
								oResult.mEnter = false;
								hit = true;
								scatteringOccured = true;
								distMax = oResult.mDist;
							}
						}
						else
						{
							raySamplePdf = currentMediumPtr->RaySamplePdf(aRay, i->mDistMin, i->mDistMax, raySamplingFlags, &raySampleRevPdf);
						}
					}

					UPBP_ASSERT(raySamplePdf > 0);
					UPBP_ASSERT(raySampleRevPdf > 0);

					VolumeSegment segment;
					segment.mDistMin = i->mDistMin;
					segment.mDistMax = distMax;					
					segment.mRaySamplePdf = raySamplePdf;					
					segment.mRaySampleRevPdf = raySampleRevPdf;					
					segment.mAttenuation = currentMediumPtr->EvalAttenuation(tempRay, segment.mDistMin, segment.mDistMax);
					segment.mEmission = currentMediumPtr->EvalEmission(tempRay, segment.mDistMin, segment.mDistMax);
					segment.mMediumID = currentMediumID;
					segmentsToIsect->push_back(segment);

					// Stack update
					Isect isect;
					while (imaginaryIsects.size() > 0 && (isect = imaginaryIsects.front()).mDist <= distMax)
					{							
						if (isect.mEnter)
							oBoundaryStack.Push(StackElement(isect.mMatID, isect.mMedID), GetMediumBoundaryPriority(isect.mMatID));
						else if (oBoundaryStack.Size() > 1)
							oBoundaryStack.Pop(StackElement(isect.mMatID, isect.mMedID), GetMediumBoundaryPriority(isect.mMatID));
						
						imaginaryIsects.pop_front();
					}

					if (scatteringOccured) break;
				}
			}
		}
		
		// Update boundary stack with hit materials
		for (Intersections::const_iterator i = hitsWithMaterial.cbegin(); i != hitsWithMaterial.cend(); ++i)
		{
			if (i->mDist > oResult.mDist) break;
			
			if (i->mEnter)
				oBoundaryStack.Push(StackElement(i->mMatID, i->mMedID), GetMediumBoundaryPriority(i->mMatID));
			else if (oBoundaryStack.Size() > 1)
				oBoundaryStack.Pop(StackElement(i->mMatID, i->mMedID), GetMediumBoundaryPriority(i->mMatID));
		}		

		// Compute shading normal?
		if (hit && !testOcclusion && !scatteringOccured)
		{
			dynamic_cast<const GeometryList *>(mRealGeometry)->mGeometry[oResult.mElementID]->computeIntersectionInfo(oResult);
		}
		
		return hit;		
	}	

    bool Occluded(
        const Pos                &aPoint,
        const Dir                &aDir,
        float                    aTMax,		
		const BoundaryStack      &aBoundaryStack) const
    {
		BoundaryStack stackCopy(aBoundaryStack);
		return Intersect(Ray(aPoint, aDir), Isect(aTMax), stackCopy, kIgnoreMediaAltogether | kOcclusionTest, 0, NULL, NULL, NULL, NULL);
	}

	bool Occluded(
        const Pos                &aPoint,
        const Dir                &aDir,
        float                    aTMax,		
		const BoundaryStack      &aBoundaryStack,
		const uint               aRaySamplingFlags,
		VolumeSegments           &oVolumeSegments) const
    {
		BoundaryStack stackCopy(aBoundaryStack);
		return Intersect(Ray(aPoint, aDir), Isect(aTMax), stackCopy, kOcclusionTest, aRaySamplingFlags, NULL, &oVolumeSegments, NULL, NULL);
	}

	// Clear boundary stack and push in the global medium without any material
	void InitBoundaryStack(BoundaryStack &oBoundaryStack) const
	{		
		oBoundaryStack.Clear();
		oBoundaryStack.Push(StackElement(-1, mGlobalMediumID), GLOBAL_MEDIUM_PRIORITY); // Global medium has implicitly the lowest possible priority
	}

	void AddToBoundaryStack(int aMatID, int aMedID, BoundaryStack &oBoundaryStack) const
	{		
		oBoundaryStack.Push(StackElement(aMatID, aMedID), GetMediumBoundaryPriority(aMatID));
	}

	void UpdateBoundaryStackOnRefract(const Isect &aIsect, BoundaryStack &oBoundaryStack) const
	{
		if (aIsect.mEnter)
		{
			if (aIsect.mMedID >= 0) oBoundaryStack.Push(StackElement(aIsect.mMatID, aIsect.mMedID), GetMediumBoundaryPriority(aIsect.mMatID));
		}
		else 
		{
			if (oBoundaryStack.Size() > 1) oBoundaryStack.PopTop();
		}
	}

    const Material& GetMaterial(const int aMaterialIdx) const
    {
        return mMaterials[aMaterialIdx];
    }

    int GetMaterialCount() const
    {
        return (int)mMaterials.size();
    }

    const AbstractLight* GetLightPtr(int aLightIdx) const
    {
        size_t idx = std::min<size_t>(aLightIdx, mLights.size()-1);
        return mLights[idx];
    }
	
    int GetLightCount() const
    {
        return (int)mLights.size();
    }

    const BackgroundLight* GetBackground() const
    {
        return mBackground;
    }	

	AbstractMedium* GetMediumPtr(int aMediumIdx) const
    {
        size_t idx = std::min<size_t>(aMediumIdx, mMedia.size()-1);
        return mMedia[idx];
    }

	AbstractMedium* GetGlobalMediumPtr() const
    {
        return mGlobalMediumID >= 0 ? mMedia[mGlobalMediumID] : nullptr;
    }

	int GetMediumBoundaryPriority(int aMaterialIdx) const
	{
		return aMaterialIdx >= 0 ? mMaterials[aMaterialIdx].mPriority : GLOBAL_MEDIUM_PRIORITY;
	}

	bool IsThisRealGeometry(int aMaterialIdx) const
	{
		return aMaterialIdx >= 0 && mMaterials[aMaterialIdx].mGeometryType == REAL;
	}

	float RelativeIOR(const Isect &aIsect, const BoundaryStack &aBoundaryStack) const
	{
		if (aIsect.IsOnSurface())
		{
			const Material& mat1 = GetMaterial(aIsect.mMatID);
			float ior1 = mat1.mIOR;

			if (ior1 < 0) return -1.0f;

			UPBP_ASSERT(!aBoundaryStack.IsEmpty());

			if (aIsect.mEnter)
			{
				float ior2 = 1.0f;
				
				int matId = aBoundaryStack.Top().mMaterialId;
				if (matId >= 0)
				{
					const Material& mat2 = GetMaterial(matId);
					if (mat2.mGeometryType != GeometryType::IMAGINARY)
						ior2 = mat2.mIOR;
				}

				UPBP_ASSERT(ior1);

				if (ior2 < 0) return -1.0f;
				else return ior2 / ior1;
			}
			else
			{
				float ior2 = 1.0f;
				
				int matId = aBoundaryStack.Size() > 1 ? aBoundaryStack.SecondFromTop().mMaterialId : aBoundaryStack.Top().mMaterialId;
				if (matId >= 0) 
				{
					const Material& mat2 = GetMaterial(matId);
					if (mat2.mGeometryType != GeometryType::IMAGINARY)
						ior2 = mat2.mIOR;		
				}

				UPBP_ASSERT(ior2);

				if (ior2 < 0) return -1.0f;
				else return ior1 / ior2;
			}
		}
		else return -1.0f;
	}

    //////////////////////////////////////////////////////////////////////////
    // Loads a Cornell Box scene
    void LoadCornellBox(
        const Vec2i &aResolution,
		const SceneConfig &aSceneConfig)
    {
		mSceneAcronym = aSceneConfig.mShortName;
		mSceneName = aSceneConfig.mLongName;

		mCamera.Setup(
            Pos(-0.0439815f, -4.12529f, 0.222539f),
            Pos(-0.03709525f, -3.126785f, 0.1683229f),
            Dir(3.73896e-4f, 0.0542148f, 0.998529f),
            Vec2f(float(aResolution[0]), float(aResolution[1])), 45.0f, 1.0f);

		//////////////////////////////////////////////////////////////////////////
		// Materials

		std::vector< Material > materialSelection;
		mMaterials.clear();
		materialSelection.resize(SceneConfig::Materials::kMaterialsCount);
        Material mat;

		// diffuse red
        mat.Reset();
        mat.mDiffuseReflectance = Rgb(0.803922f, 0.152941f, 0.152941f);
		materialSelection[SceneConfig::Materials::kDiffuseRed] = mat;

		//diffuse green
        mat.Reset();
        mat.mDiffuseReflectance = Rgb(0.156863f, 0.803922f, 0.172549f);
		materialSelection[SceneConfig::Materials::kDiffuseGreen] = mat;

		// diffuse blue
        mat.Reset();
        mat.mDiffuseReflectance = Rgb(0.156863f, 0.172549f, 0.803922f);
		materialSelection[SceneConfig::Materials::kDiffuseBlue] = mat;
		
		// diffuse white
        mat.Reset();
        mat.mDiffuseReflectance = Rgb(0.803922f, 0.803922f, 0.803922f);
		materialSelection[SceneConfig::Materials::kDiffuseWhite] = mat;
        
		// glossy white
        mat.Reset();
        mat.mDiffuseReflectance = Rgb(0.1f);
        mat.mPhongReflectance   = Rgb(0.7f);
        mat.mPhongExponent         = 90.f;
		materialSelection[SceneConfig::Materials::kGlossyWhite] = mat;
		
		// mirror
        mat.Reset();
        mat.mMirrorReflectance = Rgb(1.f);
		materialSelection[SceneConfig::Materials::kMirror] = mat;

		// water
        mat.Reset();
        mat.mMirrorReflectance  = Rgb(0.7f);
        mat.mIOR                = 1.33f;
        materialSelection[SceneConfig::Materials::kWaterMaterial] = mat;

		// ice
        mat.Reset();
        mat.mMirrorReflectance  = Rgb(0.5f);
        mat.mIOR                = 1.31f;
		materialSelection[SceneConfig::Materials::kIce] = mat;

		// glass
        mat.Reset();
        mat.mMirrorReflectance  = Rgb(1.f);
        mat.mIOR                = 1.6f;
		materialSelection[SceneConfig::Materials::kGlass] = mat;

		// for area lights
        mat.Reset();
		materialSelection[SceneConfig::Materials::kLight] = mat;

		mMaterials.insert(mMaterials.begin(), materialSelection.begin(), materialSelection.end());
		//////////////////////////////////////////////////////////////////////////
		// Media
		
		mMedia.clear();
		mMedia.resize(SceneConfig::Media::kMediaCount);
		
		// clear
		mMedia[SceneConfig::Media::kClear] = new ClearMedium();

		// water
		mMedia[SceneConfig::Media::kWaterMedium] = new HomogeneousMedium(Rgb(0.7f, 0.6f, 0.0f), Rgb(0.0f), Rgb(0.0f), MEDIUM_SURVIVAL_PROB);

		// red absorbing
		mMedia[SceneConfig::Media::kRedAbsorbing] = new HomogeneousMedium(Rgb(0.0f, 1.0f, 1.0f), Rgb(0.0f), Rgb(0.0f), MEDIUM_SURVIVAL_PROB);

		// yellow emitting
		mMedia[SceneConfig::Media::kYellowEmitting] = new HomogeneousMedium(Rgb(0.0f), Rgb(0.7f, 0.7f, 0.0f), Rgb(0.0f), MEDIUM_SURVIVAL_PROB);

		// yellow-green absorbing, emitting and scattering
		mMedia[SceneConfig::Media::kYellowGreenAbsorbingEmittingAndScattering] = new HomogeneousMedium(Rgb(0.5f, 0.0f, 0.5f), Rgb(1.0f, 1.0f, 0.0f), Rgb(0.1f, 0.1f, 0.0f), MEDIUM_SURVIVAL_PROB);

		// blue absorbing and emitting
		mMedia[SceneConfig::Media::kBlueAbsorbingAndEmitting] = new HomogeneousMedium(Rgb(0.1f, 0.1f, 0.0f), Rgb(0.0f, 0.0f, 1.0f), Rgb(0.0f), MEDIUM_SURVIVAL_PROB);

		// white isoscattering
		mMedia[SceneConfig::Media::kWhiteIsoScattering] = new HomogeneousMedium(Rgb(0.0f), Rgb(0.0f), Rgb(0.9f), MEDIUM_SURVIVAL_PROB);

		// white anisoscattering
		mMedia[SceneConfig::Media::kWhiteAnisoScattering] = new HomogeneousMedium(Rgb(0.0f), Rgb(0.0f), Rgb(0.9f), MEDIUM_SURVIVAL_PROB, 0.6f);

		// weak white isoscattering
		mMedia[SceneConfig::Media::kWeakWhiteIsoScattering] = new HomogeneousMedium(Rgb(0.0f), Rgb(0.0f), Rgb(0.1f), MEDIUM_SURVIVAL_PROB);

		// weak yellow isoscattering
		mMedia[SceneConfig::Media::kWeakYellowIsoScattering] = new HomogeneousMedium(Rgb(0.0f), Rgb(0.0f), Rgb(0.1f,0.1f,0.0f), MEDIUM_SURVIVAL_PROB);

		// weak white anisoscattering
		mMedia[SceneConfig::Media::kWeakWhiteAnisoScattering] = new HomogeneousMedium(Rgb(0.0f), Rgb(0.0f), Rgb(0.1f), MEDIUM_SURVIVAL_PROB, 0.6f);

		// reddish light
		mMedia[SceneConfig::Media::kLightReddishMedium] = new HomogeneousMedium(Rgb(0.0f, 0.02f, 0.0f), Rgb(0.0f), Rgb(0.002f,0.002f,0.0f), MEDIUM_SURVIVAL_PROB, 0.6f);

		// absorbing and anisoscattering
		mMedia[SceneConfig::Media::kAbsorbingAnisoScattering] = new HomogeneousMedium(Rgb(0.02f, 2.0f, 2.0f), Rgb(0.0f), Rgb(12.0f, 20.0f, 20.0f), MEDIUM_SURVIVAL_PROB, -0.3f);

		// global infinite medium
		mGlobalMediumID = aSceneConfig.mGlobalMedium;

        //////////////////////////////////////////////////////////////////////////
        // Geometry

		delete mRealGeometry;
		GeometryList *realGeometryList = new AcceleratedGeometryList;
        mRealGeometry = realGeometryList;

		delete mImaginaryGeometry;
        GeometryList *imaginaryGeometryList = new GeometryList;
        mImaginaryGeometry = imaginaryGeometryList;

		// Cornell box vertices
        Pos cb[8] = {
            Pos(-1.27029f,  1.30455f, -1.28002f),
            Pos( 1.28975f,  1.30455f, -1.28002f),
            Pos( 1.28975f,  1.30455f,  1.28002f),
            Pos(-1.27029f,  1.30455f,  1.28002f),
            Pos(-1.27029f, -1.25549f, -1.28002f),
            Pos( 1.28975f, -1.25549f, -1.28002f),
            Pos( 1.28975f, -1.25549f,  1.28002f),
            Pos(-1.27029f, -1.25549f,  1.28002f)
        };
		Pos centerOfFloor = (cb[0] + cb[1] + cb[4] + cb[5]) * (1.f / 4.f);
		Pos centerOfBox = (cb[0] + cb[1] + cb[2] + cb[3] + cb[4] + cb[5] + cb[6] + cb[7]) * (1.f / 8.f);

		// Spheres params
		float largeSphereRadius = 0.8f;
		Pos largeSphereCenter = centerOfFloor + Dir(0, 0, largeSphereRadius + 0.01f);
		float smallSphereRadius = 0.5f;
		Pos leftWallCenter  = (cb[0] + cb[4]) * (1.f / 2.f) + Dir(0, 0, smallSphereRadius + 0.01f);
		Pos rightWallCenter = (cb[1] + cb[5]) * (1.f / 2.f) + Dir(0, 0, smallSphereRadius + 0.01f);
		float xlen = rightWallCenter.x() - leftWallCenter.x();
		Pos leftSmallSphereCenter  = leftWallCenter  + Dir(2.f * xlen / 7.f, 0, 0);
        Pos rightSmallSphereCenter = rightWallCenter - Dir(2.f * xlen / 7.f, 0, 0);
		float overlap = 0.5f * smallSphereRadius;
		Pos bottomLeftSmallSphereCenter  = centerOfBox + Dir(-overlap, 0, -overlap);
		Pos bottomRightSmallSphereCenter = centerOfBox + Dir(overlap, 0, -overlap);
		Pos topSmallSphereCenter         = centerOfBox + Dir(0, 0, overlap);
		Pos veryLargeSphereCenter = centerOfBox;
		float veryLargeSphereRadius = 1.0f;

		// Already added geometry map
		std::vector<bool> used(SceneConfig::Geometry::kGeometryCount, false);

		// Geometry construction
		for (std::vector<SceneConfig::Element>::const_iterator i = aSceneConfig.mElements.begin(); i < aSceneConfig.mElements.end(); ++i)
		{
			if (used[i->mGeometry]) continue;
			else used[i->mGeometry] = true;

			GeometryList *geometryList = (i->mMaterial < 0) ? imaginaryGeometryList : realGeometryList;
			int priority = ( 1 + i->mMaterial ) * 10 + 1 + i->mMedium;
			int matId = SceneConfig::Materials::kMaterialsCount;
			if (i->mGeometry > SceneConfig::Geometry::kFloor)
			{
				for (; matId != mMaterials.size(); ++matId)
					if (mMaterials[matId].mPriority == priority) break;
				if (matId == mMaterials.size())
				{
					Material mat;
					mat.Reset();
					if (i->mMaterial >= 0) mat = mMaterials[i->mMaterial];
					mat.mPriority = priority;
					mat.mGeometryType = i->mMaterial >= 0 ? REAL : IMAGINARY;
					mMaterials.push_back(mat);
				}
			}
			else
				matId = i->mMaterial;

			switch (i->mGeometry)
			{
			case SceneConfig::Geometry::kLeftWall:
				geometryList->mGeometry.push_back(new Triangle(cb[3], cb[7], cb[4], matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[4], cb[0], cb[3], matId, i->mMedium));
				break;
			case SceneConfig::Geometry::kRightWall:
				geometryList->mGeometry.push_back(new Triangle(cb[1], cb[5], cb[6], matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[6], cb[2], cb[1], matId, i->mMedium));
				break;
			case SceneConfig::Geometry::kFrontFacingFrontWall:
				geometryList->mGeometry.push_back(new Triangle(cb[4], cb[5], cb[6], matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[6], cb[7], cb[4], matId, i->mMedium));
				break;
			case SceneConfig::Geometry::kBackFacingFrontWall:
				geometryList->mGeometry.push_back(new Triangle(cb[6], cb[5], cb[4], matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[4], cb[7], cb[6], matId, i->mMedium));
				break;
			case SceneConfig::Geometry::kBackWall:
				geometryList->mGeometry.push_back(new Triangle(cb[0], cb[1], cb[2], matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[2], cb[3], cb[0], matId, i->mMedium));
				break;
			case SceneConfig::Geometry::kCeiling:
				if (aSceneConfig.mLight != SceneConfig::Lights::kLightCeilingAreaBig)
				{
					geometryList->mGeometry.push_back(new Triangle(cb[2], cb[6], cb[7], matId, i->mMedium));
					geometryList->mGeometry.push_back(new Triangle(cb[7], cb[3], cb[2], matId, i->mMedium)); 
				}
				break;
			case SceneConfig::Geometry::kFloor:
				geometryList->mGeometry.push_back(new Triangle(cb[0], cb[4], cb[5], matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[5], cb[1], cb[0], matId, i->mMedium));
				break;
			case SceneConfig::Geometry::kLargeSphereMiddle:
				geometryList->mGeometry.push_back(new Sphere(largeSphereCenter, largeSphereRadius, matId, i->mMedium));
				break;
			case SceneConfig::Geometry::kSmallSphereLeft:
				geometryList->mGeometry.push_back(new Sphere(leftSmallSphereCenter, smallSphereRadius, matId, i->mMedium));				
				break;
			case SceneConfig::Geometry::kSmallSphereRight:
				geometryList->mGeometry.push_back(new Sphere(rightSmallSphereCenter, smallSphereRadius, matId, i->mMedium));
				break;
			case SceneConfig::Geometry::kSmallSphereBottomLeft:
				geometryList->mGeometry.push_back(new Sphere(bottomLeftSmallSphereCenter, smallSphereRadius, matId, i->mMedium));
				break;
			case SceneConfig::Geometry::kSmallSphereBottomRight:
				geometryList->mGeometry.push_back(new Sphere(bottomRightSmallSphereCenter, smallSphereRadius, matId, i->mMedium));
				break;
			case SceneConfig::Geometry::kSmallSphereTop:
				geometryList->mGeometry.push_back(new Sphere(topSmallSphereCenter, smallSphereRadius, matId, i->mMedium));
				break;
			case SceneConfig::Geometry::kVeryLargeSphere:
				geometryList->mGeometry.push_back(new Sphere(veryLargeSphereCenter, veryLargeSphereRadius, matId, i->mMedium));
				break;
			case SceneConfig::Geometry::kVeryLargeBox:
				const float mult = 1.0f;
				geometryList->mGeometry.push_back(new Triangle(cb[7] * mult, cb[3] * mult, cb[4] * mult, matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[0] * mult, cb[4] * mult, cb[3] * mult, matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[5] * mult, cb[1] * mult, cb[6] * mult, matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[2] * mult, cb[6] * mult, cb[1] * mult, matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[4] * mult, cb[5] * mult, cb[6] * mult, matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[6] * mult, cb[7] * mult, cb[4] * mult, matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[1] * mult, cb[0] * mult, cb[2] * mult, matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[3] * mult, cb[2] * mult, cb[0] * mult, matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[6] * mult, cb[2] * mult, cb[7] * mult, matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[3] * mult, cb[7] * mult, cb[2] * mult, matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[4] * mult, cb[0] * mult, cb[5] * mult, matId, i->mMedium));
				geometryList->mGeometry.push_back(new Triangle(cb[1] * mult, cb[5] * mult, cb[0] * mult, matId, i->mMedium));
				break;
			}
		}
		
		//////////////////////////////////////////////////////////////////////////
		// Lights

		switch (aSceneConfig.mLight)
		{
		case SceneConfig::Lights::kLightCeilingAreaBig:
			{				
				mLights.resize(2);

				AreaLight *al = new AreaLight(cb[2], cb[6], cb[7]);
				al->mIntensity = Rgb(0.95492965f);
				mLights[0] = al;

				al = new AreaLight(cb[7], cb[3], cb[2]);
				al->mIntensity = Rgb(0.95492965f);
				mLights[1] = al;

				realGeometryList->mGeometry.push_back(new Triangle(cb[2], cb[6], cb[7], SceneConfig::Materials::kLight, -1, 0));
				realGeometryList->mGeometry.push_back(new Triangle(cb[7], cb[3], cb[2], SceneConfig::Materials::kLight, -1, 1));
			}
			break;
		case SceneConfig::Lights::kLightCeilingAreaSmall:
			{
				// Box vertices
				Pos lb[8] = {
					Pos(-0.25f,  0.25f, 1.26002f),
					Pos( 0.25f,  0.25f, 1.26002f),
					Pos( 0.25f,  0.25f, 1.28002f),
					Pos(-0.25f,  0.25f, 1.28002f),
					Pos(-0.25f, -0.25f, 1.26002f),
					Pos( 0.25f, -0.25f, 1.26002f),
					Pos( 0.25f, -0.25f, 1.28002f),
					Pos(-0.25f, -0.25f, 1.28002f)
				};

				mLights.resize(2);

				AreaLight *al = new AreaLight(lb[0], lb[5], lb[4]);
				al->mIntensity = Rgb(25.03329895614464f);
				mLights[0] = al;

				al = new AreaLight(lb[5], lb[0], lb[1]);
				al->mIntensity = Rgb(25.03329895614464f);
				mLights[1] = al;

				// Back wall
				realGeometryList->mGeometry.push_back(new Triangle(lb[0], lb[2], lb[1], SceneConfig::Materials::kDiffuseWhite));
				realGeometryList->mGeometry.push_back(new Triangle(lb[2], lb[0], lb[3], SceneConfig::Materials::kDiffuseWhite));
				// Left wall
				realGeometryList->mGeometry.push_back(new Triangle(lb[3], lb[4], lb[7], SceneConfig::Materials::kDiffuseWhite));
				realGeometryList->mGeometry.push_back(new Triangle(lb[4], lb[3], lb[0], SceneConfig::Materials::kDiffuseWhite));
				// Right wall
				realGeometryList->mGeometry.push_back(new Triangle(lb[1], lb[6], lb[5], SceneConfig::Materials::kDiffuseWhite));
				realGeometryList->mGeometry.push_back(new Triangle(lb[6], lb[1], lb[2], SceneConfig::Materials::kDiffuseWhite));
				// Front wall
				realGeometryList->mGeometry.push_back(new Triangle(lb[4], lb[5], lb[6], SceneConfig::Materials::kDiffuseWhite));
				realGeometryList->mGeometry.push_back(new Triangle(lb[6], lb[7], lb[4], SceneConfig::Materials::kDiffuseWhite));
				// Floor
				realGeometryList->mGeometry.push_back(new Triangle(lb[0], lb[5], lb[4], SceneConfig::Materials::kLight, -1, 0));
				realGeometryList->mGeometry.push_back(new Triangle(lb[5], lb[0], lb[1], SceneConfig::Materials::kLight, -1, 1));			
			}
			break;
		case SceneConfig::Lights::kLightCeilingAreaSmallDistant:
			{
				const float posY = -1.5f;
				// Box vertices
				Pos lb[8] = {
					Pos(-0.25f, 0.25f + posY, 1.26002f),
					Pos(0.25f, 0.25f + posY, 1.26002f),
					Pos(0.25f, 0.25f + posY, 1.28002f),
					Pos(-0.25f, 0.25f + posY, 1.28002f),
					Pos(-0.25f, -0.25f + posY, 1.26002f),
					Pos(0.25f, -0.25f + posY, 1.26002f),
					Pos(0.25f, -0.25f + posY, 1.28002f),
					Pos(-0.25f, -0.25f + posY, 1.28002f)
				};
				mLights.resize(2);

				AreaLight *al = new AreaLight(lb[0], lb[5], lb[4]);
				al->mIntensity = Rgb(25.03329895614464f);
				mLights[0] = al;

				al = new AreaLight(lb[5], lb[0], lb[1]);
				al->mIntensity = Rgb(25.03329895614464f);
				mLights[1] = al;

				realGeometryList->mGeometry.push_back(new Triangle(lb[0], lb[5], lb[4], SceneConfig::Materials::kLight, -1, 0));
				realGeometryList->mGeometry.push_back(new Triangle(lb[5], lb[0], lb[1], SceneConfig::Materials::kLight, -1, 1));
			}
			break;
		case SceneConfig::Lights::kLightCeilingPoint:
			{
				PointLight *pl = new PointLight(Pos(0.0, -0.5, 1.0));
				pl->mIntensity = Rgb(70.f * (INV_PI_F * 0.25f));
				mLights.push_back(pl);
			}
			break;		
		case SceneConfig::Lights::kLightFacingAreaSmall:
			{
				// Rectangle vertices
				Pos lr[8] = {
					Pos(-0.25f, 0, 0.25f),
					Pos(0.25f, 0, 0.25f),
					Pos(-0.25f, 0, -0.25f),
					Pos(0.25f, 0, -0.25f)
				};

				mLights.resize(2);

				AreaLight *al = new AreaLight(lr[2], lr[3], lr[0]);
				al->mIntensity = Rgb(25.03329895614464f);
				mLights[0] = al;

				al = new AreaLight(lr[1], lr[0], lr[3]);
				al->mIntensity = Rgb(25.03329895614464f);
				mLights[1] = al;

				realGeometryList->mGeometry.push_back(new Triangle(lr[2], lr[3], lr[0], SceneConfig::Materials::kLight, -1, 0));
				realGeometryList->mGeometry.push_back(new Triangle(lr[1], lr[0], lr[3], SceneConfig::Materials::kLight, -1, 1));				
			}
			break;
		case SceneConfig::Lights::kLightFacingPoint:
			{
				PointLight *pl = new PointLight(Pos(0.0f, 0.0f, 0.0f));
				pl->mIntensity = Rgb(70.f * (INV_PI_F * 0.25f));
				mLights.push_back(pl);
			}
			break;

		case SceneConfig::Lights::kLightSun:
			{
				DirectionalLight *dl = new DirectionalLight(Dir(-1.f, 1.5f, -1.f));
				dl->mIntensity = Rgb(0.5f, 0.2f, 0.f) * 20.f;
				mLights.push_back(dl);
			}
			break;
		case SceneConfig::Lights::kLightBackground:
			{
				BackgroundLight *bl = new BackgroundLight;
				bl->mScale = 1.f;
				mLights.push_back(bl);
				mBackground = bl;
			}
			break;
		}
    }

    void BuildSceneSphere()
    {
		Pos bboxMin( 1e36f);
        Pos bboxMax(-1e36f);
        mRealGeometry->GrowBBox(bboxMin, bboxMax);
		mImaginaryGeometry->GrowBBox(bboxMin, bboxMax);

        float radius2 = (bboxMax - bboxMin).square();
		if (radius2 == 0 || radius2 > INFINITY) radius2 = 1.0f;

        mSceneSphere.mSceneCenter = (bboxMax + bboxMin) * 0.5f;
        mSceneSphere.mSceneRadius = std::sqrt(radius2) * 0.5f;		
        mSceneSphere.mInvSceneRadiusSqr = 1.f / Utils::sqr(mSceneSphere.mSceneRadius);
    }

	/// Loads scene from a selected obj file
	void LoadFromObj(const char * file, const Vec2i &aResolution)
	{
		mSceneAcronym = "obj";
		mSceneName = file;

		// Load file
		ObjReader::ObjFile obj(file);

		// Convert loader representation to small upbp representation

		//// Setup camera
		int camMat = obj.camera().material - 1;
		int camMed = camMat < 0 ? -1 : obj.materials()[camMat + 1].mediumId;
		mCamera.Setup(
			Pos(obj.camera().origin[0], obj.camera().origin[1], obj.camera().origin[2]),
			Pos(obj.camera().target[0], obj.camera().target[1], obj.camera().target[2]),
			Dir(obj.camera().roll[0], obj.camera().roll[1], obj.camera().roll[2]),
			Vec2f(float(aResolution[0]), float(aResolution[1])),
			obj.camera().horizontalFOV* 57.295779f,
			obj.camera().focalDistance, camMat, camMed);

		// Handle materials
		mMaterials.clear();
		const ObjReader::Materials & objMaterials = obj.materials();
		mMaterials.resize(objMaterials.size() - 1);
		for (unsigned int i = 1; i < objMaterials.size(); ++i)
		{
			mMaterials[i - 1].mDiffuseReflectance = Rgb(objMaterials[i].diffuse[0], objMaterials[i].diffuse[1], objMaterials[i].diffuse[2]);
			mMaterials[i - 1].mPhongReflectance = Rgb(objMaterials[i].specular[0], objMaterials[i].specular[1], objMaterials[i].specular[2]);
			mMaterials[i - 1].mMirrorReflectance = Rgb(objMaterials[i].mirror[0], objMaterials[i].mirror[1], objMaterials[i].mirror[2]);
			mMaterials[i - 1].mIOR = objMaterials[i].IOR;
			mMaterials[i - 1].mPhongExponent = objMaterials[i].shininess;
			mMaterials[i - 1].mPriority = objMaterials[i].priority;
			mMaterials[i - 1].mGeometryType = objMaterials[i].geometryType;
			if (objMaterials[i].shininess == 0.0f)
			{
				mMaterials[i - 1].mPhongReflectance = Rgb(0);
			}
			UPBP_ASSERT(objMaterials[i].priority != -1 || objMaterials[i].mediumId == -1);
		}

		// Handle medium
		for (unsigned int i = 0; i < mMedia.size(); ++i)
		{
			delete mMedia[i];
		}
		mMedia.clear();
		const ObjReader::Media & objMedia = obj.media();
		mMedia.resize(objMedia.size());
		for (unsigned int i = 0; i < objMedia.size(); ++i)
		{
			float contProb = objMedia[i].continuationProbability;
			if (contProb == -1.0f)
			{
				// Set as albedo
				float albedo0 = objMedia[i].scatteringCoef[0] / (objMedia[i].scatteringCoef[0] + objMedia[i].absorptionCoef[0]);
				float albedo1 = objMedia[i].scatteringCoef[1] / (objMedia[i].scatteringCoef[1] + objMedia[i].absorptionCoef[1]);
				float albedo2 = objMedia[i].scatteringCoef[2] / (objMedia[i].scatteringCoef[2] + objMedia[i].absorptionCoef[2]);
				float maxAlbedo = std::max(albedo0, std::max(albedo1, albedo2));
				if (Float::isNanInf(maxAlbedo))
					contProb = 1.0f;
				else
					contProb = maxAlbedo > MEDIUM_SURVIVAL_PROB ? maxAlbedo : MEDIUM_SURVIVAL_PROB;
			}
			mMedia[i] = new HomogeneousMedium(Rgb(objMedia[i].absorptionCoef[0], objMedia[i].absorptionCoef[1], objMedia[i].absorptionCoef[2]),
				Rgb(objMedia[i].emissionCoef[0], objMedia[i].emissionCoef[1], objMedia[i].emissionCoef[2]),
				Rgb(objMedia[i].scatteringCoef[0], objMedia[i].scatteringCoef[1], objMedia[i].scatteringCoef[2]),
				contProb, objMedia[i].meanCosine);
		}
		mGlobalMediumID = obj.globalMediumId();
		if (mGlobalMediumID == -1)
		{
			mMedia.push_back(new ClearMedium());
			mGlobalMediumID = (int)mMedia.size() - 1;
		}

		// Handle geometry & lights
		for (unsigned int i = 0; i < mLights.size(); ++i)
		{
			delete mLights[i];
		}
		mLights.clear();
		delete mRealGeometry;
		GeometryList *realGeometryList = new AcceleratedGeometryList;
		mRealGeometry = realGeometryList;

		delete mImaginaryGeometry;
		GeometryList *imaginaryGeometryList = new GeometryList;
		mImaginaryGeometry = imaginaryGeometryList;
		const ObjReader::Groups & objGroups = obj.groups();
		for (int i = 0; i < objGroups.size(); ++i)
		{
			const ObjReader::Material & mat = objMaterials[objGroups[i].material];
			const ObjReader::Indices & trIds = objGroups[i].triangles;
			for (int t = 0; t < trIds.size(); ++t)
			{
				// Get obj triangle
				const ObjReader::Triangle & objTriangle = obj.triangles()[trIds[t]];
				const float * p0 = &(obj.vertices()[0]) + 3 * objTriangle.vindices[0];
				const float * p1 = &(obj.vertices()[0]) + 3 * objTriangle.vindices[1];
				const float * p2 = &(obj.vertices()[0]) + 3 * objTriangle.vindices[2];
				const float * n0 = 0, *n1 = 0, *n2 = 0, *t0 = 0, *t1 = 0, *t2 = 0;
				n0 = obj.normals().size() > 3 * objTriangle.nindices[0] ? &(obj.normals()[0]) + 3 * objTriangle.nindices[0] : 0;
				n1 = obj.normals().size() > 3 * objTriangle.nindices[1] ? &(obj.normals()[0]) + 3 * objTriangle.nindices[1] : 0;
				n2 = obj.normals().size() > 3 * objTriangle.nindices[2] ? &(obj.normals()[0]) + 3 * objTriangle.nindices[2] : 0;
				t0 = obj.texcoords().size() > 2 * objTriangle.tindices[0] ? &(obj.texcoords()[0]) + 2 * objTriangle.tindices[0] : 0;
				t1 = obj.texcoords().size() > 2 * objTriangle.tindices[1] ? &(obj.texcoords()[0]) + 2 * objTriangle.tindices[1] : 0;
				t2 = obj.texcoords().size() > 2 * objTriangle.tindices[2] ? &(obj.texcoords()[0]) + 2 * objTriangle.tindices[2] : 0;
				// Create our triangle
				Triangle * triangle = new Triangle(Pos(p0[0], p0[1], p0[2]), Pos(p1[0], p1[1], p1[2]), Pos(p2[0], p2[1], p2[2]), objGroups[i].material - 1, mat.mediumId);
				// Set normals
				if (n0 != 0 || n1 != 0 || n2 != 0)
				{
					triangle->n[0] = Dir(n0[0], n0[1], n0[2]);
					triangle->n[1] = Dir(n1[0], n1[1], n1[2]);
					triangle->n[2] = Dir(n2[0], n2[1], n2[2]);
					// Check face normal
					Dir mdl = 1.0f / 3 * (triangle->n[0] + triangle->n[1] + triangle->n[2]);
					if (dot(triangle->mNormal, mdl) < 0.0f)
					{
						triangle->mNormal = -triangle->mNormal;
					}
				}
				if (t0 != 0 || t1 != 0 || t2 != 0)
				{
					// Set texture coordinates
					triangle->t[0] = Vec2f(t0[0], t0[1]);
					triangle->t[1] = Vec2f(t1[0], t1[1]);
					triangle->t[2] = Vec2f(t2[0], t2[1]);
				}

				// Set light
				if (mat.isEmissive)
				{
					triangle->lightID = (int)mLights.size();
					AreaLight * a = new AreaLight(triangle->p[0], triangle->p[1], triangle->p[2]);
					a->mIntensity = Rgb(mat.emmissive[0], mat.emmissive[1], mat.emmissive[2]);

					int lightMat = mat.enclosingMatId - 1;
					int lightMed = lightMat < 0 ? -1 : objMaterials[lightMat + 1].mediumId;
					a->mMatID = lightMat;
					a->mMedID = lightMed;

					mLights.push_back(a);
				}
				if (mat.geometryType == REAL)
				{
					realGeometryList->mGeometry.push_back(triangle);
				}
				else // ObjReader::IMAGINARY
				{
					imaginaryGeometryList->mGeometry.push_back(triangle);
				}
			}
		}
		// Handle additional lights
		mBackground = NULL;
		const ObjReader::Lights & objLights = obj.lights();
		for (unsigned int i = 0; i < objLights.size(); ++i)
		{
			switch (objLights[i].lightType)
			{
			case ObjReader::POINT:
			{
									 PointLight * light = new PointLight(Pos(objLights[i].position[0], objLights[i].position[1], objLights[i].position[2]));
									 light->mIntensity = Rgb(objLights[i].emission[0], objLights[i].emission[1], objLights[i].emission[2]);
									 mLights.push_back(light);
			}
				break;
			case ObjReader::BACKGROUND:
			{
										  if (!mBackground)
										  {
											  BackgroundLight * light = new BackgroundLight();
											  light->mScale = 1.0f;
											  if (objLights[i].envMap.length() > 0) light->mEnvMap = new EnvMap(objLights[i].envMap, objLights[i].envMapRotate, objLights[i].envMapScale);
											  else light->mBackgroundColor = Rgb(objLights[i].emission[0], objLights[i].emission[1], objLights[i].emission[2]);
											  mBackground = light;
											  mLights.push_back(light);
										  }
			}
				break;
			case ObjReader::DIRECTIONAL:
			{
										   DirectionalLight * light = new DirectionalLight(Dir(objLights[i].position[0], objLights[i].position[1], objLights[i].position[2]));
										   light->mIntensity = Rgb(objLights[i].emission[0], objLights[i].emission[1], objLights[i].emission[2]);
										   mLights.push_back(light);
			}
				break;
			}
		}
	}

public:

    AbstractGeometry              *mRealGeometry;
	AbstractGeometry              *mImaginaryGeometry;
    Camera                        mCamera;
    std::vector<Material>         mMaterials;
	std::vector<AbstractMedium*>  mMedia;
	int                           mGlobalMediumID;
	float                         mMaxBeamLengthInGlobalMedium;
    std::vector<AbstractLight*>   mLights;
    SceneSphere                   mSceneSphere;
    BackgroundLight*              mBackground;

    std::string                   mSceneName;
    std::string                   mSceneAcronym;
};

#endif //__SCENE_HXX__