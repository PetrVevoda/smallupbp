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

#ifndef __LIGHTS_HXX__
#define __LIGHTS_HXX__

#include <vector>
#include <cmath>

#include "..\Misc\Utils2.hxx"
#include "EnvMap.hxx"

struct SceneSphere
{
    // Center of the scene's bounding sphere
    Pos mSceneCenter;
    // Radius of the scene's bounding sphere
    float mSceneRadius;
    // 1.f / (mSceneRadius^2)
    float mInvSceneRadiusSqr;
};

class AbstractLight
{
public:

	AbstractLight(int aMatID = -1, int aMedID = -1) : mMatID(aMatID), mMedID(aMedID)
	{}

    /* \brief Illuminates a given point in the scene.
     *
     * Given a point and two random samples (e.g., for position on area lights),
     * this method returns direction from point to light, distance,
     * PDF of having chosen this direction (e.g., 1 / area).
     * Optionally also returns PDF of emitting particle in this direction,
     * and cosine from lights normal (helps with PDF of hitting the light,
     * but set to 1 for point lights).
     *
     * Returns radiance.
     */
    virtual Rgb Illuminate(
        const SceneSphere &aSceneSphere,
        const Pos         &aReceivingPosition,
        const Vec2f       &aRndTuple,
        Dir               &oDirectionToLight,
        float             &oDistance,
        float             &oDirectPdfW,
        float             *oEmissionPdfW = NULL,
        float             *oCosAtLight = NULL) const = 0;

    /* \brief Emits particle from the light.
     *
     * Given two sets of random numbers (e.g., position and direction on area light),
     * this method generates a position and direction for light particle, along
     * with the PDF.
     *
     * Can also supply PDF (w.r.t. area) of choosing this position when calling
     * Illuminate. Also provides cosine on the light (this is 1 for point lights etc.).
     *
     * Returns "energy" that particle carries
     */
    virtual Rgb Emit(
        const SceneSphere &aSceneSphere,
        const Vec2f       &aDirRndTuple,
        const Vec2f       &aPosRndTuple,
        Pos               &oPosition,
        Dir               &oDirection,
        float             &oEmissionPdfW,
        float             *oDirectPdfA,
        float             *oCosThetaLight) const = 0;

    /* \brief Returns radiance for ray randomly hitting the light
     *
     * Given ray direction and hitpoint, it returns radiance.
     * Can also provide area PDF of sampling hitpoint in Illuminate,
     * and of emitting particle along the ray (in opposite direction).
     */
    virtual Rgb GetRadiance(
        const SceneSphere &aSceneSphere,
        const Dir         &aRayDirection,
        const Pos         &aHitPoint,
        float             *oDirectPdfA = NULL,
        float             *oEmissionPdfW = NULL) const = 0;

    // Whether the light has a finite extent (area, point) or not (directional, env. map)
    virtual bool IsFinite() const = 0;

    // Whether the light has delta function (point, directional) or not (area)
    virtual bool IsDelta() const = 0;

public:
	int mMatID; // Id of material enclosing the light (-1 for light in global medium)
	int mMedID; // Id of medium enclosing the light (-1 for light in global medium)
};

//////////////////////////////////////////////////////////////////////////
class AreaLight : public AbstractLight
{
public:

    AreaLight(
        const Pos &aP0,
        const Pos &aP1,
        const Pos &aP2)
    {
        p0 = aP0;
        e1 = aP1 - aP0;
        e2 = aP2 - aP0;

        Dir normal = cross(e1, e2);
        float len    = normal.size();
        mInvArea     = 2.f / len;
        mFrame.SetFromZ(normal);
    }

    virtual Rgb Illuminate(
        const SceneSphere &/*aSceneSphere*/,
        const Pos         &aReceivingPosition,
        const Vec2f       &aRndTuple,
        Dir               &oDirectionToLight,
        float             &oDistance,
        float             &oDirectPdfW,
        float             *oEmissionPdfW = NULL,
        float             *oCosAtLight = NULL) const
    {
        const Vec2f uv = SampleUniformTriangle(aRndTuple);
        const Pos lightPoint = p0 + e1 * uv.get(0) + e2 * uv.get(1);

        oDirectionToLight     = lightPoint - aReceivingPosition;
        const float distSqr   = oDirectionToLight.square();
        oDistance             = std::sqrt(distSqr);
        oDirectionToLight     = oDirectionToLight / oDistance;

        const float cosNormalDir = dot(mFrame.Normal(), -oDirectionToLight);

        // too close to, or under, tangent
        if(cosNormalDir < EPS_COSINE)
        {
            return Rgb(0.f);
        }

        oDirectPdfW = mInvArea * distSqr / cosNormalDir;

        if(oCosAtLight)
            *oCosAtLight = cosNormalDir;

        if(oEmissionPdfW)
            *oEmissionPdfW = mInvArea * cosNormalDir * INV_PI_F;

        return mIntensity;
    }

    virtual Rgb Emit(
        const SceneSphere &/*aSceneSphere*/,
        const Vec2f       &aDirRndTuple,
        const Vec2f       &aPosRndTuple,
        Pos               &oPosition,
        Dir               &oDirection,
        float             &oEmissionPdfW,
        float             *oDirectPdfA,
        float             *oCosThetaLight) const
    {
        const Vec2f uv = SampleUniformTriangle(aPosRndTuple);
        oPosition = p0 + e1 * uv.get(0) + e2 * uv.get(1);

        Dir localDirOut = SampleCosHemisphereW(aDirRndTuple, &oEmissionPdfW);

        oEmissionPdfW *= mInvArea;

        // cannot really not emit the particle, so just bias it to the correct angle
        localDirOut.z() = std::max(localDirOut.z(), EPS_COSINE);
        oDirection      = mFrame.ToWorld(localDirOut);

        if(oDirectPdfA)
            *oDirectPdfA = mInvArea;

        if(oCosThetaLight)
            *oCosThetaLight = localDirOut.z();

        return mIntensity * localDirOut.z();
    }

    virtual Rgb GetRadiance(
        const SceneSphere &/*aSceneSphere*/,
        const Dir         &aRayDirection,
        const Pos         &aHitPoint,
        float             *oDirectPdfA = NULL,
        float             *oEmissionPdfW = NULL) const
    {
        const float cosOutL = std::max(0.f, dot(mFrame.Normal(), -aRayDirection));

        if(cosOutL == 0)
            return Rgb(0);

        if(oDirectPdfA)
            *oDirectPdfA = mInvArea;

        if(oEmissionPdfW)
        {
            *oEmissionPdfW = CosHemispherePdfW(mFrame.Normal(), -aRayDirection);
            *oEmissionPdfW *= mInvArea;
        }

        return mIntensity;
    }
    // Whether the light has a finite extent (area, point) or not (directional, env. map)
    virtual bool IsFinite() const { return true; }

    // Whether the light has delta function (point, directional) or not (area)
    virtual bool IsDelta() const { return false; }

public:

    Pos p0;
	Dir e1, e2;
    Frame mFrame;
    Rgb mIntensity;
    float mInvArea;
};

//////////////////////////////////////////////////////////////////////////
class DirectionalLight : public AbstractLight
{
public:
    DirectionalLight(const Dir& aDirection)
    {
        mFrame.SetFromZ(aDirection);
    }

    virtual Rgb Illuminate(
        const SceneSphere &aSceneSphere,
        const Pos         &/*aReceivingPosition*/,
        const Vec2f       &/*aRndTuple*/,
        Dir               &oDirectionToLight,
        float             &oDistance,
        float             &oDirectPdfW,
        float             *oEmissionPdfW = NULL,
        float             *oCosAtLight = NULL) const
    {
        oDirectionToLight     = -mFrame.Normal();
        oDistance             = 1e36f;
        oDirectPdfW           = 1.f;

        if(oCosAtLight)
            *oCosAtLight = 1.f;

        if(oEmissionPdfW)
            *oEmissionPdfW = ConcentricDiscPdfA() * aSceneSphere.mInvSceneRadiusSqr;

        return mIntensity;
    }

    virtual Rgb Emit(
        const SceneSphere &aSceneSphere,
        const Vec2f       &/*aDirRndTuple*/,
        const Vec2f       &aPosRndTuple,
        Pos               &oPosition,
        Dir               &oDirection,
        float             &oEmissionPdfW,
        float             *oDirectPdfA,
        float             *oCosThetaLight) const
    {
        const Vec2f xy = SampleConcentricDisc(aPosRndTuple);

        oPosition = aSceneSphere.mSceneCenter +
            aSceneSphere.mSceneRadius * (
            -mFrame.Normal() + mFrame.Binormal() * xy.get(0) + mFrame.Tangent() * xy.get(1));

        oDirection = mFrame.Normal();
        oEmissionPdfW = ConcentricDiscPdfA() * aSceneSphere.mInvSceneRadiusSqr;

        if(oDirectPdfA)
            *oDirectPdfA = 1.f;

        // Not used for infinite or delta lights
        if(oCosThetaLight)
            *oCosThetaLight = 1.f;

        return mIntensity;
    }

    virtual Rgb GetRadiance(
        const SceneSphere &/*aSceneSphere*/,
        const Dir         &/*aRayDirection*/,
        const Pos         &/*aHitPoint*/,
        float             *oDirectPdfA = NULL,
        float             *oEmissionPdfW = NULL) const
    {
        return Rgb(0);
    }

    // Whether the light has a finite extent (area, point) or not (directional, env. map)
    virtual bool IsFinite() const { return false; }
    
    // Whether the light has delta function (point, directional) or not (area)
    virtual bool IsDelta() const  { return true; }

public:

    Frame mFrame;
    Rgb mIntensity;
};


//////////////////////////////////////////////////////////////////////////
class PointLight : public AbstractLight
{
public:

    PointLight(const Pos& aPosition)
    {
        mPosition = aPosition;
    }

    virtual Rgb Illuminate(
        const SceneSphere &/*aSceneSphere*/,
        const Pos         &aReceivingPosition,
        const Vec2f       &aRndTuple,
        Dir               &oDirectionToLight,
        float             &oDistance,
        float             &oDirectPdfW,
        float             *oEmissionPdfW = NULL,
        float             *oCosAtLight = NULL) const
    {
        oDirectionToLight   = mPosition - aReceivingPosition;
        const float distSqr = oDirectionToLight.square();
        oDirectPdfW         = distSqr;
        oDistance           = std::sqrt(distSqr);
        oDirectionToLight   = oDirectionToLight / oDistance;

        if(oCosAtLight)
            *oCosAtLight = 1.f;

        if(oEmissionPdfW)
            *oEmissionPdfW = UniformSpherePdfW();

        return mIntensity;
    }

    virtual Rgb Emit(
        const SceneSphere &/*aSceneSphere*/,
        const Vec2f       &aDirRndTuple,
        const Vec2f       &/*aPosRndTuple*/,
        Pos               &oPosition,
        Dir               &oDirection,
        float             &oEmissionPdfW,
        float             *oDirectPdfA,
        float             *oCosThetaLight) const
    {
        oPosition  = mPosition;
        oDirection = SampleUniformSphereW(aDirRndTuple, &oEmissionPdfW);

        if(oDirectPdfA)
            *oDirectPdfA = 1.f;

        // Not used for infinite or delta lights
        if(oCosThetaLight)
            *oCosThetaLight = 1.f;

        return mIntensity;
    }

    virtual Rgb GetRadiance(
        const SceneSphere &/*aSceneSphere*/,
        const Dir         &/*aRayDirection*/,
        const Pos         &/*aHitPoint*/,
        float             *oDirectPdfA = NULL,
        float             *oEmissionPdfW = NULL) const
    {
        return Rgb(0);
    }
    
    // Whether the light has a finite extent (area, point) or not (directional, env. map)
    virtual bool IsFinite() const { return true; }
    
    // Whether the light has delta function (point, directional) or not (area)
    virtual bool IsDelta() const  { return true; }

public:

    Pos mPosition;
    Rgb mIntensity;
};


//////////////////////////////////////////////////////////////////////////
class BackgroundLight : public AbstractLight
{
public:
    BackgroundLight()
    {
        mBackgroundColor = Rgb(135, 206, 250) / Rgb(255.f);
		mEnvMap = 0;
		mScale = 1.f;
    }

	virtual ~BackgroundLight()
	{
		delete(mEnvMap);
	}

    virtual Rgb Illuminate(
        const SceneSphere &aSceneSphere,
        const Pos         &aReceivingPosition,
        const Vec2f       &aRndTuple,
        Dir               &oDirectionToLight,
        float             &oDistance,
        float             &oDirectPdfW,
        float             *oEmissionPdfW = NULL,
        float             *oCosAtLight = NULL) const
    {
		Rgb radiance;

		if (mEnvMap)
		{
			oDirectionToLight = mEnvMap->Sample(aRndTuple, oDirectPdfW, &radiance);
			radiance *= mScale;
		}
		else
		{
			oDirectionToLight = SampleUniformSphereW(aRndTuple, &oDirectPdfW);
			radiance = mBackgroundColor * mScale;
		}

        // This stays even with image sampling
        oDistance = 1e36f;
        if(oEmissionPdfW)
            *oEmissionPdfW = oDirectPdfW * ConcentricDiscPdfA() *
                aSceneSphere.mInvSceneRadiusSqr;

        if(oCosAtLight)
            *oCosAtLight = 1.f;

        return radiance;
    }

    virtual Rgb Emit(
        const SceneSphere &aSceneSphere,
        const Vec2f       &aDirRndTuple,
        const Vec2f       &aPosRndTuple,
        Pos               &oPosition,
        Dir               &oDirection,
        float             &oEmissionPdfW,
        float             *oDirectPdfA,
        float             *oCosThetaLight) const
    {
        float directPdf;
		Rgb radiance;	
		
		if (mEnvMap)
		{
			oDirection = -mEnvMap->Sample(aDirRndTuple, directPdf, &radiance);
			radiance *= mScale;
		}
		else
		{
			oDirection = SampleUniformSphereW(aDirRndTuple, &directPdf);
			radiance = mBackgroundColor * mScale;
		}

        // Stays even with image sampling
        const Vec2f xy = SampleConcentricDisc(aPosRndTuple);

        Frame frame;
        frame.SetFromZ(oDirection);
        
        oPosition = aSceneSphere.mSceneCenter + aSceneSphere.mSceneRadius * (
            -oDirection + frame.Binormal() * xy.get(0) + frame.Tangent() * xy.get(1));

        oEmissionPdfW = directPdf * ConcentricDiscPdfA() *
            aSceneSphere.mInvSceneRadiusSqr;

        // For background we lie about PDF being in area measure
        if(oDirectPdfA)
            *oDirectPdfA = directPdf;
        
        // Not used for infinite or delta lights
        if(oCosThetaLight)
            *oCosThetaLight = 1.f;

        return radiance;
    }

    virtual Rgb GetRadiance(
        const SceneSphere &aSceneSphere,
        const Dir         &aRayDirection,
        const Pos         &/*aHitPoint*/,
        float             *oDirectPdfA = NULL,
        float             *oEmissionPdfW = NULL) const
    {		
		float directPdf;
		Rgb radiance;

		if (mEnvMap)
		{
			radiance = mScale * mEnvMap->Lookup(aRayDirection, &directPdf);
		}
		else
		{
			directPdf = UniformSpherePdfW();
			radiance  = mBackgroundColor * mScale;
		}

        const float positionPdf = ConcentricDiscPdfA() *
            aSceneSphere.mInvSceneRadiusSqr;

        if(oDirectPdfA)
            *oDirectPdfA   = directPdf;

        if(oEmissionPdfW)
            *oEmissionPdfW = directPdf * positionPdf;
        
        return radiance;
    }

    // Whether the light has a finite extent (area, point) or not (directional, env. map)
    virtual bool IsFinite() const { return false; }
    
    // Whether the light has delta function (point, directional) or not (area)
    virtual bool IsDelta() const  { return false; }

public:

    Rgb mBackgroundColor;
	EnvMap* mEnvMap;
    float mScale;
};
#endif //__LIGHTS_HXX__
