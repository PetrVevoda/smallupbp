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

#ifndef __FRAMEBUFFER_HXX__
#define __FRAMEBUFFER_HXX__

#include <vector>
#include <cmath>
#include <fstream>
#include <string.h>

#include "Utils2.hxx"
#include "..\..\..\OpenEXR\ImfOutputFile.h"
#include "..\..\..\OpenEXR\ImfInputFile.h"
#include "..\..\..\OpenEXR\ImfChannelList.h"
#include "..\..\..\OpenEXR\ImfStringAttribute.h"
#include "..\..\..\OpenEXR\ImfMatrixAttribute.h"
#include "..\..\..\OpenEXR\ImfArray.h"

/**
 * @brief	A framebuffer.
 * 			
 * 			Accumulates and saves an image.
 */
class Framebuffer
{
public:

    /**
     * @brief	Default constructor.
     */
    Framebuffer()
    {}

	//////////////////////////////////////////////////////////////////////////
    // Accumulation

    /**
     * @brief	Adds the given color to pixel containing the given position.
     *
     * @param	aSample	Position of the sample.
     * @param	aColor 	Color of the sample.
     */
    void AddColor(
        const Vec2f& aSample,
        const Rgb& aColor)
    {
        if(aSample.get(0) < 0 || aSample.get(0) >= mResolution.get(0))
            return;

        if(aSample.get(1) < 0 || aSample.get(1) >= mResolution.get(1))
            return;

        int x = int(aSample.get(0));
        int y = int(aSample.get(1));

        mColor[x + y * mResX] = mColor[x + y * mResX] + aColor;
    }

	//////////////////////////////////////////////////////////////////////////
    // Methods for framebuffer operations

    /**
     * @brief	Setups the framebuffer.
     *
     * @param	aResolution	Resolution of the framebuffer.
     */
    void Setup(const Vec2f& aResolution)
    {
        mResolution = aResolution;
        mResX = int(aResolution.get(0));
        mResY = int(aResolution.get(1));
        mColor.resize(mResX * mResY);
        Clear();
    }

    /**
     * @brief	Clears the framebuffer.
     */
    void Clear()
    {
        memset(&mColor[0], 0, sizeof(Rgb) * mColor.size());
    }

    /**
     * @brief	Adds other framebuffer.
     *
     * @param	aOther	The other framebuffer to add.
     */
    void Add(const Framebuffer& aOther)
    {
        for(size_t i=0; i<mColor.size(); i++)
            mColor[i] = mColor[i] + aOther.mColor[i];
    }

	/**
	 * @brief	Adds other framebuffer scaled.
	 *
	 * @param	aOther	The other framebuffer to add.
	 * @param	aScale	The factor to scale the other values.
	 */
	void AddScaled(const Framebuffer& aOther, float aScale)
	{
		for (size_t i = 0; i < mColor.size(); i++)
			mColor[i] = mColor[i] + aOther.mColor[i] * aScale;
	}

    /**
     * @brief	Scales values in this framebuffer.
     *
     * @param	aScale	The scale.
     */
    void Scale(float aScale)
    {
        for(size_t i=0; i<mColor.size(); i++)
            mColor[i] = mColor[i] * Rgb(aScale);
    }

	//////////////////////////////////////////////////////////////////////////
    // Statistics

    /**
     * @brief	Total luminance in this framebuffer.
     *
     * @return	The total luminance.
     */
    float TotalLuminance()
    {
        float lum = 0;

        for(int y=0; y<mResY; y++)
        {
            for(int x=0; x<mResX; x++)
            {
                lum += Luminance(mColor[x + y*mResX]);
            }
        }

        return lum;
    }

	//////////////////////////////////////////////////////////////////////////
    // Saving

    /**
     * @brief	Saves this framebuffer as an image in PPM format.
     *
     * @param	aFilename	Name of the image file.
     * @param	aGamma   	(Optional) the gamma.
     */
    void SavePPM(
        const char *aFilename,
        float       aGamma = 1.f)
    {
        const float invGamma = 1.f / aGamma;

        std::ofstream ppm(aFilename);
        ppm << "P3" << std::endl;
        ppm << mResX << " " << mResY << std::endl;
        ppm << "255" << std::endl;

        Rgb *ptr = &mColor[0];

        for(int y=0; y<mResY; y++)
        {
            for(int x=0; x<mResX; x++)
            {
                ptr = &mColor[x + y*mResX];
                int r = int(std::pow(ptr->r(), invGamma) * 255.f);
                int g = int(std::pow(ptr->g(), invGamma) * 255.f);
                int b = int(std::pow(ptr->b(), invGamma) * 255.f);

                ppm << std::min(255, std::max(0, r)) << " "
                    << std::min(255, std::max(0, g)) << " "
                    << std::min(255, std::max(0, b)) << " ";
            }

            ppm << std::endl;
        }
    }

    /**
     * @brief	Saves this framebuffer as an image in PFM format.
     *
     * @param	aFilename	Name of the image file.
     */
    void SavePFM(const char* aFilename)
    {
        std::ofstream ppm(aFilename, std::ios::binary);
        ppm << "PF" << std::endl;
        ppm << mResX << " " << mResY << std::endl;
        ppm << "-1" << std::endl;

        ppm.write(reinterpret_cast<const char*>(&mColor[0]),
            mColor.size() * sizeof(Dir));
    }

    /**
     * @brief	A bitmap header.
     */
    struct BmpHeader
    {
        uint   mFileSize;        //!< Size of file in bytes.
        uint   mReserved01;      //!< 2x 2 reserved bytes.
        uint   mDataOffset;      //!< Offset in bytes where data can be found (54).

        uint   mHeaderSize;      //!< 40B.
        int    mWidth;           //!< Width in pixels.
        int    mHeight;          //!< Height in pixels.

        short  mColorPlates;     //!< Must be 1.
        short  mBitsPerPixel;    //!< We use 24bpp.
        uint   mCompression;     //!< We use BI_RGB ~ 0, uncompressed.
        uint   mImageSize;       //!< mWidth x mHeight x 3B.
        uint   mHorizRes;        //!< Pixels per meter (75dpi ~ 2953ppm).
        uint   mVertRes;         //!< Pixels per meter (75dpi ~ 2953ppm).
        uint   mPaletteColors;   //!< Not using palette - 0.
        uint   mImportantColors; //!< 0 - all are important.
    };

	/**
	 * @brief	Saves this framebuffer as an image in a format corresponding to the given file name
	 * 			(BMP, HDR, OpenEXR).
	 *
	 * @param	aFilename	Name of the image file.
	 * @param	aGamma   	(Optional) the gamma.
	 */
	void Save(const char * aFilename, float aGamma = 2.2f)
	{
		Save(std::string(aFilename), aGamma);
	}

	/**
	 * @brief	Saves this framebuffer as an image in a format corresponding to the given file name
	 * 			(BMP, HDR, OpenEXR).
	 *
	 * @param [in,out]	aFilename	Name of the image file.
	 * @param	aGamma			 	(Optional) the gamma.
	 */
	void Save(std::string & aFilename, float aGamma = 2.2f)
	{
		std::string extension = aFilename.substr(aFilename.length() - 3, 3);
		if (extension == "bmp")
			SaveBMP(aFilename.c_str(), aGamma /*gamma*/);
		else if (extension == "hdr")
			SaveHDR(aFilename.c_str());
		else if (extension == "exr")
			SaveOpenEXR(aFilename.c_str());
		else
		{
			std::cerr << "Error: used unknown extension " << extension << std::endl;
			exit(2);
		}
	}

private:

    /**
     * @brief	Saves this framebuffer as an image in BMP format.
     *
     * @param	aFilename	Name of the image file.
     * @param	aGamma   	(Optional) the gamma.
     */
    void SaveBMP(
        const char *aFilename,
        float       aGamma = 1.f)
    {
        std::ofstream bmp(aFilename, std::ios::binary);
        BmpHeader header;
        bmp.write("BM", 2);
        header.mFileSize   = uint(sizeof(BmpHeader) + 2) + mResX * mResX * 3;
        header.mReserved01 = 0;
        header.mDataOffset = uint(sizeof(BmpHeader) + 2);
        header.mHeaderSize = 40;
        header.mWidth      = mResX;
        header.mHeight     = mResY;
        header.mColorPlates     = 1;
        header.mBitsPerPixel    = 24;
        header.mCompression     = 0;
        header.mImageSize       = mResX * mResY * 3;
        header.mHorizRes        = 2953;
        header.mVertRes         = 2953;
        header.mPaletteColors   = 0;
        header.mImportantColors = 0;

        bmp.write((char*)&header, sizeof(header));

        const float invGamma = 1.f / aGamma;
        for(int y=0; y<mResY; y++)
        {
            for(int x=0; x<mResX; x++)
            {
                // BMP is stored from bottom up.
                const Rgb &rgbF = mColor[x + (mResY-y-1)*mResX];
                typedef unsigned char byte;
                float gammaBgr[3];
                gammaBgr[0] = std::pow(rgbF.b(), invGamma) * 255.f;
                gammaBgr[1] = std::pow(rgbF.g(), invGamma) * 255.f;
                gammaBgr[2] = std::pow(rgbF.r(), invGamma) * 255.f;

                byte bgrB[3];
                bgrB[0] = byte(std::min(255.f, std::max(0.f, gammaBgr[0])));
                bgrB[1] = byte(std::min(255.f, std::max(0.f, gammaBgr[1])));
                bgrB[2] = byte(std::min(255.f, std::max(0.f, gammaBgr[2])));

                bmp.write((char*)&bgrB, sizeof(bgrB));
            }
        }
    }

    /**
     * @brief	Saves this framebuffer as an image in HDR format.
     *
     * @param	aFilename	Name of the image file.
     */
    void SaveHDR(const char* aFilename)
    {
        std::ofstream hdr(aFilename, std::ios::binary);

        hdr << "#?RADIANCE" << '\n';
        hdr << "# SmallUPBP" << '\n';
        hdr << "FORMAT=32-bit_rle_rgbe" << '\n' << '\n';
        hdr << "-Y " << mResY << " +X " << mResX << '\n';

        for(int y=0; y<mResY; y++)
        {
            for(int x=0; x<mResX; x++)
            {
                typedef unsigned char byte;
                byte rgbe[4] = {0,0,0,0};

                const Rgb &rgbF = mColor[x + y*mResX];
                float v = std::max(rgbF.r(), std::max(rgbF.g(), rgbF.b()));

                if(v >= 1e-32f)
                {
                    int e;
                    v = float(frexp(v, &e) * 256.f / v);
                    rgbe[0] = byte(rgbF.r() * v);
                    rgbe[1] = byte(rgbF.g() * v);
                    rgbe[2] = byte(rgbF.b() * v);
                    rgbe[3] = byte(e + 128);
                }

                hdr.write((char*)&rgbe[0], 4);
            }
        }
    }

	/**
     * @brief	Saves this framebuffer as an image in OpenEXR format.
     *
     * @param	aFilename	Name of the image file.
     */
	void SaveOpenEXR(const char* aFilename)
	{
		try
		{
			Imf::Header header(mResX, mResY);
			header.channels().insert("R", Imf::Channel(Imf::FLOAT));
			header.channels().insert("G", Imf::Channel(Imf::FLOAT));
			header.channels().insert("B", Imf::Channel(Imf::FLOAT));

			Imf::OutputFile file(aFilename, header);

			Imf::FrameBuffer frameBuffer;

			char * pixels = reinterpret_cast<char *>(&mColor[0]);
			frameBuffer.insert("R",					// name
				Imf::Slice(Imf::FLOAT,			// type
				pixels,		// base
				sizeof (Rgb)* 1,		// xStride
				sizeof (Rgb)* mResX));	// yStride

			frameBuffer.insert("G",					// name
				Imf::Slice(Imf::FLOAT,			// type
				pixels + sizeof(float),		// base
				sizeof (Rgb)* 1,		// xStride
				sizeof (Rgb)* mResX));	// yStride

			frameBuffer.insert("B",					// name
				Imf::Slice(Imf::FLOAT,			// type
				pixels + sizeof(float)* 2,		// base
				sizeof (Rgb)* 1,		// xStride
				sizeof (Rgb)* mResX));	// yStride

			file.setFrameBuffer(frameBuffer);
			file.writePixels(mResY);
		}
		catch (...)
		{
			std::cerr << "Error: saving file failed" << std::endl;
			exit(2);
		}
	}

    std::vector<Rgb>   mColor;      //!< The color
    Vec2f              mResolution; //!< Resolution of the framebuffer.
    int                mResX;       //!< Width of the framebuffer.
    int                mResY;       //!< Height of the framebuffer.
};

#endif //__FRAMEBUFFER_HXX__