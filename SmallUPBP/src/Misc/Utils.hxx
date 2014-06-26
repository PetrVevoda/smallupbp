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

#ifndef __UTILS_HXX__
#define __UTILS_HXX__

#include <xmmintrin.h>
#include <immintrin.h>
#include <sstream>
#include <iomanip>
#include <set>

#include "Sse.hxx"

//////////////////////////////////////////////////////////////////////////
// Utilities for manipulation with numbers

// Float utilities.
namespace Float {

    /**
     * @brief	Returns true if float is neither negative/positive infinity nor NaN.
     *
     * @param	x	The float to test.
     *
     * @return	true if neither negative/positive infinity nor NaN.
     */
    INLINE bool isReal(const float x) {
        return ((int&)x & 0x7fffffff) < 0x7f800000;
    }

    /**
     * @brief	Returns true if the float is Not a Number.
     *
     * @param	x	The float to test.
     *
     * @return	true if Not a Number.
     */
    INLINE bool isNan(const float x) {
        return _isnanf(x) != 0;
    }

	/**
	 * @brief	Returns true if the float is infinity.
	 *
	 * @param	x	The float to test.
	 *
	 * @return	true if infinity.
	 */
	INLINE bool isInf(const float x) {
		const int intVal = *((int*)&x);
		return (intVal & 0x7FFFFFFF) == 0x7F800000;
	}

	/**
	 * @brief	Returns true if the float is Not a Number or infinity.
	 *
	 * @param	x	The float to test.
	 *
	 * @return	true if Not a Number or infinity.
	 */
	INLINE bool isNanInf(const float x) {
		const int intVal = *((int*)&x);
		return (intVal & 0x7F800000) == 0x7F800000;
	}

	/**
	 * @brief	Returns true if the float is Not a Number, infinity or negative.
	 *
	 * @param	x	The float to test.
	 *
	 * @return	true if Not a Number, infinity or negative.
	 */
	INLINE bool isNanInfNeg(const float x) {
		const uint intVal = *((uint*)&x);
		return ((intVal & 0x7F800000) == 0x7F800000) | (x < 0.f);
	}

	/**
	 * @brief	Returns true if the float is greater than zero.
	 *
	 * @param	x	The float to test.
	 *
	 * @return	true if positive.
	 */
	INLINE bool isPositive(const float x) {
		return x > 1e-20f;
	}
}

// Some general support functions.
namespace Utils {

    /**
     * @brief	Minimum from two values.
     *
     * @typeparam	T	Type of the values to compare.
     * @param	x	First value to compare.
     * @param	y	Second value to compare.
     *
     * @return	The minimum value.
     */
    template <class T> 
    INLINE T min(const T x, const T y) {
        return (x > y) ? y : x;
    }

    /**
     * @brief	Minimum from two floats.
     *
     * @param	x	First value to compare.
     * @param	y	Second value to compare.
     *
     * @return	The minimum value.
     */
    template <> 
    INLINE float min(const float x, const float y) {
        // because of NaNs
        UPBP_ASSERT(!Float::isNan(x) && !Float::isNan(y));
        return (x > y) ? y : x;
    }

    /**
     * @brief	Maximum from two values.
     *
     * @typeparam	T	Type of the values to compare.
     * @param	x	First value to compare.
     * @param	y	Second value to compare.
     *
     * @return	The maximum value.
     */
    template <class T>
    INLINE T max(const T x, const T y) {
        return (x > y) ? x : y;
    }

    /**
     * @brief	Maximum from two floats.
     *
     * @param	x	First value to compare.
     * @param	y	Second value to compare.
     *
     * @return	The maximum value.
     */
    template <> 
    INLINE float max(const float x, const float y) {
        // because of NaNs
        UPBP_ASSERT(!Float::isNan(x) && !Float::isNan(y));
        return (x > y) ? x : y;
    }

    /**
     * @brief	Maximum from 3 values.
     *
     * @typeparam	T	Type of the values to compare.
     * @param	x	First value to compare.
     * @param	y	Second value to compare.
     * @param	z	Third value to compare.
     *
     * @return	The maximum value.
     */
    template <class T> 
    INLINE T max(const T x, const T y, const T z) {
        return (x > y) ? ((x > z) ? x : z) : ((y > z) ? y : z);
    }

    /**
     * @brief	Maximum from 3 floats.
     *
     * @param	x	First value to compare.
     * @param	y	Second value to compare.
     * @param	z	Third value to compare.
     *
     * @return	The maximum value.
     */
    template <> 
    INLINE float max(const float x, const float y, const float z) {
        // because of NaNs
        UPBP_ASSERT(!Float::isNan(x) && !Float::isNan(y) && !Float::isNan(z));
        return (x > y) ? ((x > z) ? x : z) : ((y > z) ? y : z);
    }

    /**
     * @brief	Minimum from 3 values.
     *
     * @typeparam	T	Type of the values to compare.
     * @param	x	First value to compare.
     * @param	y	Second value to compare.
     * @param	z	Third value to compare.
     *
     * @return	The minimum value.
     */
    template <class T>
    INLINE T min(const T x, const T y, const T z) {
        return (x < y) ? ((x < z) ? x : z) : ((y < z) ? y : z);
    }

    /**
     * @brief	Minimum from 3 floats.
     *
     * @param	x	First value to compare.
     * @param	y	Second value to compare.
     * @param	z	Third value to compare.
     *
     * @return	The minimum value.
     */
    template <>
    INLINE float min(const float x, const float y, const float z) {
        // because of NaNs
        UPBP_ASSERT(!Float::isNan(x) && !Float::isNan(y) && !Float::isNan(z));
        return (x < y) ? ((x < z) ? x : z) : ((y < z) ? y : z);
    }

    /**
     * @brief	Maximum from 4 values.
     *
     * @typeparam	T	Type of the values to compare.
     * @param	x	First value to compare.
     * @param	y	Second value to compare.
     * @param	z	Third value to compare.
     * @param	w	Fourth value to compare.
     *
     * @return	The maximum value.
     */
    template <class T> 
    INLINE T max(const T x, const T y, const T z, const T w) {
        return Utils::max(Utils::max(x, y), Utils::max(z, w));
    }

    /**
     * @brief	Minimum from 4 values.
     *
     * @typeparam	T	Type of the values to compare.
     * @param	x	First value to compare.
     * @param	y	Second value to compare.
     * @param	z	Third value to compare.
     * @param	w	Fourth value to compare.
     *
     * @return	The minimum value.
     */
    template <class T>
    INLINE T min(const T x, const T y, const T z, const T w) {
        return Utils::min(Utils::min(x, y), Utils::min(z, w));
    }

    /**
     * @brief	Returns value clamped to be less or equal to maximum and greater or equal to minimum.
     *
     * @typeparam	T	Type of the values.
     * @param	value  	The value.
     * @param	minimum	The minimum.
     * @param	maximum	The maximum.
     *
     * @return	The clamped value.
     */
    template<class T>
    INLINE T clamp(const T value, const T minimum, const T maximum) {
        UPBP_ASSERT(minimum <= maximum);
        return Utils::max(minimum, Utils::min(value, maximum));
    }

	/**
	 * @brief	Returns square of the given value.
	 *
	 * @typeparam	T	Type of the value to square.
	 * @param	value	The value to square.
	 *
	 * @return	The squared value.
	 */
	template<class T>
	INLINE T sqr(const T value) { 
		return value * value; 
	}

    /**
     * @brief	SSE-accelerated helper function for computing multiple square roots at once.
     *
     * @param	in0				The first value to compute square root for.
     * @param [in,out]	out0	The first result.
     * @param	in1				The second value to compute square root for.
     * @param [in,out]	out1	The second result.
     */
    INLINE void sqrt(const float in0, float& out0, const float in1, float& out1) {
        Float4 temp;
        temp[0] = in0;
        temp[1] = in1;
        temp = temp.sqrt();
        out0 = temp[0];
        out1 = temp[1];
    }

    /**
     * @brief	SSE-accelerated helper function for computing multiple square roots at once.
     *
     * @param	in0				The first value to compute square root for.
     * @param [in,out]	out0	The first result.
     * @param	in1				The second value to compute square root for.
     * @param [in,out]	out1	The second result.
     * @param	in2				The third value to compute square root for.
     * @param [in,out]	out2	The third result.
     */
    INLINE void sqrt(const float in0, float& out0, const float in1, float& out1, const float in2, float& out2) {
        Float4 temp;
        temp[0] = in0;
        temp[1] = in1;
        temp[2] = in2;
        temp = temp.sqrt();
        out0 = temp[0];
        out1 = temp[1];
        out2 = temp[2];
    }

    /**
     * @brief	SSE-accelerated helper function for computing multiple square roots at once.
     *
     * @param	in0				The first value to compute square root for.
     * @param [in,out]	out0	The first result.
     * @param	in1				The second value to compute square root for.
     * @param [in,out]	out1	The second result.
     */
    INLINE void sqrtFast(const float in0, float& out0, const float in1, float& out1) {
        Float4 temp;
        temp[0] = in0;
        temp[1] = in1;
        temp = temp.sqrtFast();
        out0 = temp[0];
        out1 = temp[1];
    }

    /**
     * @brief	SSE-accelerated helper function for computing multiple square roots at once.
     *
     * @param	in0				The first value to compute square root for.
     * @param [in,out]	out0	The first result.
     * @param	in1				The second value to compute square root for.
     * @param [in,out]	out1	The second result.
     * @param	in2				The third value to compute square root for.
     * @param [in,out]	out2	The third result.
     */
    INLINE void sqrtFast(const float in0, float& out0, const float in1, float& out1, const float in2, float& out2) {
        Float4 temp;
        temp[0] = in0;
        temp[1] = in1;
        temp[2] = in2;
        temp = temp.sqrtFast();
        out0 = temp[0];
        out1 = temp[1];
        out2 = temp[2];
    }

    /**
     * @brief	SSE-accelerated helper function for computing multiple cosines at once.
     *
     * @param	in0				The first value to compute cosine for.
     * @param [in,out]	out0	The first result.
     * @param	in1				The second value to compute cosine for.
     * @param [in,out]	out1	The second result.
     */
    INLINE void cos(const float in0, float& out0, const float in1, float& out1) {
        Float4 temp;
        temp[0] = in0;
        temp[1] = in1;
        temp = Float4(Ff::cos(temp.data));
        out0 = temp[0];
        out1 = temp[1];
    }

    /**
     * @brief	SSE-accelerated helper function for computing multiple cosines at once.
     *
     * @param	in0				The first value to compute cosine for.
     * @param [in,out]	out0	The first result.
     * @param	in1				The second value to compute cosine for.
     * @param [in,out]	out1	The second result.
     * @param	in2				The third value to compute cosine for.
     * @param [in,out]	out2	The third result.
     */
    INLINE void cos(const float in0, float& out0, const float in1, float& out1, const float in2, float& out2) {
        Float4 temp;
        temp[0] = in0;
        temp[1] = in1;
        temp[2] = in2;
        temp = Float4(Ff::cos(temp.data));
        out0 = temp[0];
        out1 = temp[1];
        out2 = temp[2];
    }

    /**
     * @brief	SSE-accelerated helper function for computing multiple sines at once.
     *
     * @param	in0				The first value to compute sine for.
     * @param [in,out]	out0	The first result.
     * @param	in1				The second value to compute sine for.
     * @param [in,out]	out1	The second result.
     */
    INLINE void sin(const float in0, float& out0, const float in1, float& out1) {
        Float4 temp;
        temp[0] = in0;
        temp[1] = in1;
        temp = Float4(Ff::sin(temp.data));
        out0 = temp[0];
        out1 = temp[1];
    }

    /**
     * @brief	SSE-accelerated helper function for computing multiple sines at once.
     *
     * @param	in0				The first value to compute sine for.
     * @param [in,out]	out0	The first result.
     * @param	in1				The second value to compute sine for.
     * @param [in,out]	out1	The second result.
     * @param	in2				The third value to compute sine for.
     * @param [in,out]	out2	The third result.
     */
    INLINE void sin(const float in0, float& out0, const float in1, float& out1, const float in2, float& out2) {
        Float4 temp;
        temp[0] = in0;
        temp[1] = in1;
        temp[2] = in2;
        temp = Float4(Ff::sin(temp.data));
        out0 = temp[0];
        out1 = temp[1];
        out2 = temp[2];
    }

    /**
     * @brief	SSE-accelerated helper function for computing multiple inverse of square root at once.
     *
     * @param	in0				The first value to compute inverse of square root for.
     * @param [in,out]	out0	The first result.
     * @param	in1				The second value to compute inverse of square root for.
     * @param [in,out]	out1	The second result.
     */
    INLINE void invSqrt(const float in0, float& out0, const float in1, float& out1) {
        Float4 temp;
        temp[0] = in0;
        temp[1] = in1;
        temp = temp.invSqrt();
        out0 = temp[0];
        out1 = temp[1];
    }

    /**
     * @brief	SSE-accelerated helper function for computing multiple inverse of square root at once.
     *
     * @param	in0				The first value to compute inverse of square root for.
     * @param [in,out]	out0	The first result.
     * @param	in1				The second value to compute inverse of square root for.
     * @param [in,out]	out1	The second result.
     * @param	in2				The third value to compute inverse of square root for.
     * @param [in,out]	out2	The third result.
     */
    INLINE void invSqrt(const float in0, float& out0, const float in1, float& out1, const float in2, float& out2) {
        Float4 temp;
        temp[0] = in0;
        temp[1] = in1;
        temp[2] = in2;
        temp = temp.invSqrt();
        out0 = temp[0];
        out1 = temp[1];
        out2 = temp[2];
    }

    /**
     * @brief	SSE-accelerated helper function for computing multiple values at once.
     *
     * @param	in0				The first value to compute inverse of square root for.
     * @param [in,out]	out0	The first result.
     * @param	in1				The second value to compute inverse of square root for.
     * @param [in,out]	out1	The second result.
     */
    INLINE void invSqrtFast(const float in0, float& out0, const float in1, float& out1) {
        Float4 temp;
        temp[0] = in0;
        temp[1] = in1;
        temp = temp.invSqrtFast();
        out0 = temp[0];
        out1 = temp[1];
    }

    /**
     * @brief	SSE-accelerated helper function for computing multiple inverse of square root at once.
     *
     * @param	in0				The first value to compute inverse of square root for.
     * @param [in,out]	out0	The first result.
     * @param	in1				The second value to compute inverse of square root for.
     * @param [in,out]	out1	The second result.
     * @param	in2				The third value to compute inverse of square root for.
     * @param [in,out]	out2	The third result.
     */
    INLINE void invSqrtFast(const float in0, float& out0, const float in1, float& out1, const float in2, float& out2) {
        Float4 temp;
        temp[0] = in0;
        temp[1] = in1;
        temp[2] = in2;
        temp = temp.invSqrtFast();
        out0 = temp[0];
        out1 = temp[1];
        out2 = temp[2];
    }
}

#endif //__UTILS_HXX__