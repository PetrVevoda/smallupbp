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

#ifndef __RENDERER_HXX__
#define __RENDERER_HXX__

#include <vector>
#include <cmath>

#include "..\Beams\Beamdensity.hxx"
#include "..\Misc\Framebuffer.hxx"
#include "..\Misc\DebugImages.hxx"
#include "..\Scene\Scene.hxx"

class AbstractRenderer
{
public:

    AbstractRenderer(const Scene& aScene) : mScene(aScene)
    {
        mMinPathLength = 0;
        mMaxPathLength = 2;
		mCameraTracingTime = 0;
        mIterations = 0;
        mFramebuffer.Setup(aScene.mCamera.mResolution);
    }

    virtual ~AbstractRenderer(){}

    virtual void RunIteration(int aIteration) = 0;

    void GetFramebuffer(Framebuffer& oFramebuffer)
    {
        oFramebuffer = mFramebuffer;

        if(mIterations > 0)
            oFramebuffer.Scale(1.f / mIterations);
    }

	Framebuffer & GetFramebufferUnscaled()
	{
		return mFramebuffer;
	}

	// Setups internal debug images
	void SetupDebugImages(DebugImages &debugImages)
	{
		mDebugImages.Setup(debugImages);
	}

	// Accumulate debug images
	void AccumulateDebugImages(DebugImages & aDebugImages)
	{
		aDebugImages.Accumulate(mDebugImages, mIterations);
	}

	// Setup beam density
	void SetupBeamDensity(const BeamDensity::ImgType aBeamDensType, const Vec2f & aResolution, const float aBeamDensMax)
	{
		mBeamDensity.Setup(aBeamDensType, aResolution, aBeamDensMax);
	}

	// Accumulate beam density
	void AccumulateBeamDensity(BeamDensity & aBeamDensity)
	{
		aBeamDensity.Accumulate(mBeamDensity);
	}

    //! Whether this renderer was used at all
    bool WasUsed() const { return mIterations > 0; }

public:

    uint         mMaxPathLength;
    uint         mMinPathLength;
	float        mCameraTracingTime;

protected:

    int          mIterations;
    Framebuffer  mFramebuffer;
    const Scene& mScene;
	DebugImages  mDebugImages;
	BeamDensity  mBeamDensity;
};

#endif //__RENDERER_HXX__