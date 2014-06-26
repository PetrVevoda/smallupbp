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

#ifndef __ENVMAP_HXX__
#define __ENVMAP_HXX__

#include "..\..\..\OpenEXR\ImfRgbaFile.h"
#include "Distribution.hxx"

class Image {
public:
	Image(int nw, int nh) {
		w = nw;
		h = nh;
		d = new Rgb[w*h];
	}

    ~Image() {
        Free();
    }

    void Free() {
        if(d) delete d;
        d = 0;
        w = h = 0;
    }

    Rgb& ElementAt(int x, int y) {
        return d[w*y+x];
    }

    Rgb& ElementAt(int idx) {
        return d[idx];
    }

	const Rgb& ElementAt(int x, int y) const {
        return d[w*y+x];
    }

    const Rgb& ElementAt(int idx) const {
        return d[idx];
    }

    int Width() const { return w; }
    int Height() const { return h; }

    Rgb* d;
    int w;
    int h;
};

class EnvMap
{
public:
	// Expects absolute path of an OpenEXR file with an environment map with latitude-longitude mapping.
	EnvMap(const std::string filename, float rotate, float scale)
	{
		mImage = 0;
		mDistribution = 0;
		mNorm = 0.5f * INV_PI_F * INV_PI_F;

		try
		{
			mImage = LoadImage(filename.c_str(), rotate, scale);
			mDistribution = ConvertImageToPdf(mImage);
			std::cout << "Loading : " << filename << std::endl;
		}
		catch (...)
		{
			std::cerr << "Error: environment map loading failed" << std::endl;
			exit(2);
		}
	}

	~EnvMap()
	{
		delete(mImage);
		delete(mDistribution);
	}

	// Samples direction on unit sphere proportionally to the luminance of the map. Returns its PDF and optionally radiance.
	Dir Sample(
		const Vec2f &aSamples,
		float       &oPdfW,
		Rgb         *oRadiance = NULL) const
	{
		float uv[2]; float pdf;
		mDistribution->SampleContinuous(aSamples[0], aSamples[1], uv, &pdf);

		UPBP_ASSERT(pdf > 0);

		oPdfW = mNorm * pdf / sinTheta(uv[1], mImage->Height());
		
		Dir direction = LatLong2Dir(uv[0], uv[1]);
		
		if (oRadiance)
			*oRadiance = LookupRadiance(uv[0], uv[1], mImage);
		
		return direction;
	}

	// Gets radiance stored for the given direction and optionally its PDF. The direction must be non-zero but not necessarily normalized.
	Rgb Lookup(
		const Dir &aDirection,
		float     *oPdfW = NULL) const
	{
		UPBP_ASSERT(aDirection.x() != 0 || aDirection.y() != 0 || aDirection.z() != 0);
		Dir normDir = aDirection.getNormalized();
		Vec2f uv = Dir2LatLong(normDir);
		Rgb radiance = LookupRadiance(uv[0],uv[1], mImage);

		if (oPdfW)
		{
			Vec2f uv = Dir2LatLong(normDir);					
			*oPdfW = mNorm * mDistribution->Pdf(uv[0], uv[1]) / sinTheta(uv[1], mImage->Height());
			if (*oPdfW == 0.0f) radiance = Rgb(0);
		}

		return radiance;
	}

private:
	// Loads, scales and rotates an environment map from an OpenEXR image on the given absolute path.
	Image* LoadImage(const char *filename, float rotate, float scale) const
	{
		Imf::RgbaInputFile file(filename, 1);
		Imath::Box2i dw = file.dataWindow();

		int width = dw.max.x - dw.min.x + 1;
		int height = dw.max.y - dw.min.y + 1;
		Imf::Rgba* imHalf = new Imf::Rgba[height*width];

		file.setFrameBuffer(imHalf - dw.min.x - dw.min.y * width, 1, width);
		file.readPixels(dw.min.y, dw.max.y);

		Image* image = new Image(width, height);

		int c = 0;
		int iRot = (int)(rotate * width);
		for (int j = 0; j < image->Height(); j++) {
			for (int i = 0; i < image->Width(); i++) {
				int x = i + iRot;
				if (x >= width) x -= width;
				image->ElementAt(x, j)[0] = imHalf[c].r * scale;
				image->ElementAt(x, j)[1] = imHalf[c].g * scale;
				image->ElementAt(x, j)[2] = imHalf[c].b * scale;
				c++;
			}
		}

		delete imHalf;

		return image;
	}

	// Converts luminance of the given environment map to 2D distribution with latitude-longitude mapping.
	Distribution2D* ConvertImageToPdf(const Image* image) const
	{
		int height = image->Height();
		int width = height + height; // height maps to PI, width maps to 2PI		
		
		float *data = new float[width * height];

		for (int r = 0; r < height; ++r) 
		{
			float v = (float)(r + 0.5f) / (float)height;
			float sinTheta = sinf(PI_F * v);
			int colOffset = r * width;
        
			for (int c = 0; c < width; ++c) 
			{
				float u = (float)(c + 0.5f) / (float)width;
				data[c + colOffset] = sinTheta * Luminance(image->ElementAt(c, r));
			}
		}

		return new Distribution2D(data, width, height);
	}

	// Returns direction on unit sphere such that its longitude equals 2*PI*u and its latitude equals PI*v.
	Dir LatLong2Dir(float u, float v) const
	{
		float phi = u * 2 * PI_F;
		float theta = v * PI_F;

		float sinTheta = sin(theta);

		return Dir(-sinTheta * cos(phi), sinTheta * sin(phi), cos(theta));
	}

	// Returns vector [u,v] such that the longitude of the given direction equals 2*PI*u and its latitude equals PI*v. The direction must be non-zero and normalized.
	Vec2f Dir2LatLong(const Dir &direction) const
	{
		float phi = (direction.x() != 0 || direction.y() != 0) ? atan2f(direction.y(), direction.x()) : 0;
		float theta = acosf(direction.z());

		float u = Utils::clamp<float>(0.5 - phi * 0.5f * INV_PI_F, 0, 1);
		float v = Utils::clamp<float>(theta * INV_PI_F, 0, 1);

		return Vec2f(u, v);
	}

	// Returns radiance for the given lat long coordinates. Does bilinear filtering.
	Rgb LookupRadiance(float u, float v, const Image* image) const
	{
		int width = image->Width();
		int height = image->Height();

		float xf = u * width;
		float yf = v * height;

		int xi1 = Utils::clamp<int>((int)xf, 0, width - 1);
		int yi1 = Utils::clamp<int>((int)yf, 0, height - 1);

		int xi2 = xi1 == width - 1 ? xi1 : xi1 + 1;
		int yi2 = yi1 == height - 1 ? yi1 : yi1 + 1;

		float tx = xf - (float)xi1;
		float ty = yf - (float)yi1;

		return (1 - ty) * ((1 - tx) * image->ElementAt(xi1, yi1) + tx * image->ElementAt(xi2, yi1))
			+ ty * ((1 - tx) * image->ElementAt(xi1, yi2) + tx * image->ElementAt(xi2, yi2));
	}

	// Returns sine of latitude for a midpoint of a pixel in a map of the given height corresponding to v. Never returns zero.
	float sinTheta(const float v, const float height) const
	{
		float result;
		
		if (v < 1)
			result = sinf(PI_F * (float)((int)(v * height) + 0.5f) / (float)height);
		else 
			result = sinf(PI_F * (float)((height - 1) + 0.5f) / (float)height);

		UPBP_ASSERT(result > 0 && result <= 1);

		return result;
	}

	Image* mImage;				    // The environment map
	Distribution2D* mDistribution;	// Environment map converted to 2D distribution	
	float mNorm;					// PDF normalization factor
};

#endif //__ENVMAP_HXX__