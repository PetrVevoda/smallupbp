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

#ifndef __CAMERA_HXX__
#define __CAMERA_HXX__

#include <vector>
#include <cmath>

#include "..\Path\Ray.hxx"
#include "..\Structs\Mat4f.hxx"
#include "..\Structs\Vector3.hxx"

class Camera
{
public:

    void Setup(
        const Pos   &aOrigin,
        const Pos   &aTarget,
        const Dir   &aRoll,
        const Vec2f &aResolution,
        float       aHorizontalFOV,
		float       aFocalDist,
		int         aMatID = -1,
		int         aMedID = -1)
    {
		mOrigin = Pos(aOrigin);
		mDirection = (aTarget - aOrigin).getNormalized();

        mResolution = aResolution;

		const Dir forward = mDirection;
		const Dir up      = cross(aRoll, -forward).getNormalized();
        const Dir left    = cross(-forward, up);

		const Dir origin(aOrigin.x(), aOrigin.y(), aOrigin.z());

        const Dir pos(
            dot(up, origin),
            dot(left, origin),
            dot(-forward, origin));

		Mat4f worldToCamera = Mat4f::Identity();
        worldToCamera.SetRow(0, up,       -pos.x());
        worldToCamera.SetRow(1, left,     -pos.y());
        worldToCamera.SetRow(2, -forward, -pos.z());

        const Mat4f perspective = Mat4f::Perspective(aHorizontalFOV, 0.1f, 10000.f);
        const Mat4f worldToNScreen = perspective * worldToCamera;
        const Mat4f nscreenToWorld = Invert(worldToNScreen);

		float aspect = aResolution.x / aResolution.y;

        mWorldToRaster  =
			Mat4f::Scale(Dir(aResolution.x * 0.5f, aResolution.x * 0.5f, 0)) *
            Mat4f::Translate(Dir(1.f, 1.f / aspect, 0)) 
			* worldToNScreen;

        mRasterToWorld  = nscreenToWorld *
			Mat4f::Translate(Dir(-1.f, -1.f / aspect, 0)) *
            Mat4f::Scale(Dir(2.f / aResolution.x, 2.f / aResolution.x, 0));

        const float tanHalfAngle = std::tan(aHorizontalFOV * PI_F / 360.f);
        mImagePlaneDist = aResolution.x / (2.f * tanHalfAngle); 
				
		mMatID = aMatID;
		mMedID = aMedID;
    }

    int RasterToIndex(const Vec2f &aPixelCoords) const
    {
        return int(std::floor(aPixelCoords.get(0)) + std::floor(aPixelCoords.get(1)) * mResolution.get(0));
    }

    Vec2f IndexToRaster(const int &aPixelIndex) const
    {
        const float y = std::floor(aPixelIndex / mResolution.get(0));
        const float x = float(aPixelIndex) - y * mResolution.get(0);
        return Vec2f(x, y);
    }

    Pos RasterToWorld(const Vec2f &aRasterXY) const
    {
        return mRasterToWorld.TransformPoint(Pos(aRasterXY.get(0), aRasterXY.get(1), 0));
    }

    Vec2f WorldToRaster(const Pos &aWorldPos) const
    {
        Pos temp = mWorldToRaster.TransformPoint(aWorldPos);
        return Vec2f(temp.x(), temp.y());
    }

    // returns false when raster position is outside screen space
    bool CheckRaster(const Vec2f &aRasterPos) const
    {
        return aRasterPos.get(0) >= 0 && aRasterPos.get(1) >= 0 &&
            aRasterPos.get(0) < mResolution.get(0) && aRasterPos.get(1) < mResolution.get(1);
    }

    Ray GenerateRay(const Vec2f &aRasterXY) const
    {
        const Pos worldRaster = RasterToWorld(aRasterXY);

        Ray res;
		res.origin = mOrigin;
		res.direction = (worldRaster - res.origin).getNormalized();
        return res;
    }

public:

	Pos   mOrigin;
	Dir   mDirection;
    Vec2f mResolution;
	Mat4f mRasterToWorld;
    Mat4f mWorldToRaster;
    float mImagePlaneDist;	
	int   mMatID;
	int   mMedID;	
};

#endif //__CAMERA_HXX__