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

#ifndef __RNG_HXX__
#define __RNG_HXX__

#include <vector>
#include <cmath>

#include "..\Structs\Vector3.hxx"


#if defined(_MSC_VER)
#   if (_MSC_VER < 1600)
#       define LEGACY_RNG
#   endif
#endif

#if !defined(LEGACY_RNG)

#include <random>

/**
 * @brief	Random number generator.
 * 			
 * 			If compiled for C++11 uses Mersenne Twister otherwise Tiny Encryption Algorithm-based random number generator.
 */
class Rng
{
public:

    /**
     * @brief	Constructor.
     *
     * @param	aSeed	(Optional) the seed.
     */
    Rng(int aSeed = 1234):
        mRng(aSeed)
    {}

    /**
     * @brief	Gets a random integer.
     *
     * @return	Random integer.
     */
    int GetInt()
    {
        return mDistInt(mRng);
    }

	/**
     * @brief	Gets a random integer.
     *
     * @return	Random integer.
     */
    uint GetUint()
    {
        return mDistUint(mRng);
    }

	/**
     * @brief	Gets a random unsigned integer.
     *
     * @return	Random unsigned integer.
     */
    float GetFloat()
    {
        return mDistFloat(mRng);
    }

    /**
     * @brief	Gets a vector of two random floats.
     *
     * @return	Vector of two random floats.
     */
    Vec2f GetVec2f()
    {
        float a = GetFloat();
        float b = GetFloat();

        return Vec2f(a, b);
    }

	/**
     * @brief	Gets a random direction.
     *
     * @return	Random direction.
     */
    Dir GetVec3f()
    {
        float a = GetFloat();
        float b = GetFloat();
        float c = GetFloat();

        return Dir(a, b, c);
    }

private:

    std::mt19937_64 mRng;   //!< Underlying Mersenne Twister.

    std::uniform_int_distribution<int>    mDistInt;   //!< Uniform integer distribution (for generating integers).
    std::uniform_int_distribution<uint>   mDistUint;  //!< Uniform integer distribution (for generating unsigned integers).
    std::uniform_real_distribution<float> mDistFloat; //!< Uniform real distribution (for generating floats).
};

#else

template<unsigned int rounds>
class TeaImplTemplate
{
public:
    void Reset(
        uint aSeed0,
        uint aSeed1)
    {
        mState0 = aSeed0;
        mState1 = aSeed1;
    }

    uint GetImpl(void)
    {
        unsigned int sum=0;
        const unsigned int delta=0x9e3779b9U;

        for (unsigned int i=0; i<rounds; i++)
        {
            sum+=delta;
            mState0+=((mState1<<4)+0xa341316cU) ^ (mState1+sum) ^ ((mState1>>5)+0xc8013ea4U);
            mState1+=((mState0<<4)+0xad90777dU) ^ (mState0+sum) ^ ((mState0>>5)+0x7e95761eU);
        }

        return mState0;
    }

private:

    uint mState0, mState1;
};

typedef TeaImplTemplate<6>  TeaImpl;

template<typename RandomImpl>
class RandomBase
{
public:

    RandomBase(int aSeed = 1234)
    {
        mImpl.Reset(uint(aSeed), 5678);
    }

    uint  GetUint()
    {
        return getImpl();
    }

    float GetFloat()
    {
        return (float(GetUint()) + 1.f) * (1.0f / 4294967297.0f);
    }

    Vec2f GetVec2f   (void)
    {
        // cannot do return Vec2f(getF32(), getF32()) because the order is not ensured
        float a = GetFloat();
        float b = GetFloat();
        return Vec2f(a, b);
    }

    Dir GetVec3f   (void)
    {
        float a = GetFloat();
        float b = GetFloat();
        float c = GetFloat();
        return Dir(a, b, c);
    }

    //////////////////////////////////////////////////////////////////////////
    void StoreState(
        uint *oState1,
        uint *oState2)
    {
        mImpl.StoreState(oState1, oState2);
    }

    void LoadState(
        uint aState1,
        uint aState2,
        uint aDimension)
    {
        mImpl.LoadState(aState1, aState2, aDimension);
    }

protected:

    uint getImpl(void)
    {
        return mImpl.GetImpl();
    }

private:

    RandomImpl mImpl;
};

typedef RandomBase<TeaImpl> Rng;

#endif

#endif //__RNG_HXX__