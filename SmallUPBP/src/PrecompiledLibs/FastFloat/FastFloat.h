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

#pragma once
/*~API~*/

#include <xmmintrin.h>
#include <emmintrin.h>
#include <math.h>

#define SSE_LIBRARY_BUILD
#define FF_INLINE __forceinline

// http://gruntthepeon.free.fr/ssemath/
namespace MathFun {
    #include "sse_mathfun.h"
}

// http://code.google.com/p/ut-sse/source/browse/trunk/examples/particleFilter/sse/?r=2
namespace SseWrapper {
    #include "sseMath.h"
}

// http://jrfonseca.blogspot.com/2008/09/fast-sse2-pow-tables-or-polynomials.html
namespace FastPow {
    #include "pow.h"
}

#pragma warning(push)
#pragma warning(disable:4238)

// Contains optimized math routines, with SSE versions.
namespace Ff {
    
    FF_INLINE __m128 sqrt(const __m128 in) {
        return _mm_sqrt_ps(in);
    }

    // Gives approximate results!
    FF_INLINE __m128 sqrtFast(const __m128 in) {
        return _mm_rcp_ps(_mm_rsqrt_ps(in));
    }

    // Gives approximate results!
    FF_INLINE __m128 invSqrtFast(const __m128 in) {
        return _mm_rsqrt_ps(in);
    }
    FF_INLINE __m128 inv(const __m128 in) {
        return _mm_div_ps(_mm_set1_ps(1.f), in);
    }
    FF_INLINE __m128 sin(const __m128 in) {
        //return MathFun::sin_ps(in);
        return SseWrapper::sin(in).data;
    }
    FF_INLINE __m128 cos(const __m128 in) {
        //return MathFun::cos_ps(in);
        return SseWrapper::cos(in).data;
    }
    FF_INLINE __m128 log(const __m128 in) {
        return MathFun::log_ps(in);
    }
    FF_INLINE __m128 exp(const __m128 in) {
        return MathFun::exp_ps(in);
    }

    // Calculates sine and cosine together
    FF_INLINE void sincos(const __m128 in, __m128& outSin, __m128& outCos) {
        MathFun::sincos_ps(in, &outSin, &outCos);
    }    
    FF_INLINE __m128 acos(const __m128 in) {
        return SseWrapper::_mm_acos_ps(in);
    }
    FF_INLINE __m128 atan(const __m128 in) {
        return SseWrapper::atan(in).data;
    }    

    // Gives approximate results!
    FF_INLINE __m128 powFast(const __m128 base, const float exponent) {
        return FastPow::fastPow(base, _mm_set1_ps(exponent));
    }

    // Gives approximate results!
    FF_INLINE __m128 powFast(const __m128 base, const __m128 exponent) {
        return FastPow::fastPow(base, exponent);
    }

    // Gives approximate results!
    FF_INLINE __m128 exp2fast(const __m128 x) {
        return FastPow::exp2f4(x);
    }

    // Gives approximate results!
    FF_INLINE __m128 log2fast(const __m128 x) {
        return FastPow::log2f4(x);
    }
    

    // Gives approximate results!
    FF_INLINE float sqrtFast(const float in) {
        __m128 temp = _mm_rcp_ss(_mm_rsqrt_ss(_mm_set1_ps(in)));
        return (float&)temp;
    }

    // Gives approximate results!
    FF_INLINE float invSqrtFast(const float in) {
        __m128 temp = _mm_rsqrt_ss(_mm_set1_ps(in));
        return (float&)temp;
    }
    FF_INLINE float sqrt(const float in) {
        return ::sqrtf(in);
    }
    FF_INLINE float invSqrt(const float in) {
        return 1.f/sqrtf(in);
    }

    FF_INLINE void sincos(const float in, float& outSin, float& outCos) {
        __m128 temp1, temp2;
        MathFun::sincos_ps(_mm_set1_ps(in), &temp1, &temp2);
        outSin = (float&)temp1;
        outCos = (float&)temp2;   
    }
    FF_INLINE float atan2(const float x, const float y) { 
        //__m128 temp = SseWrapper::atan2(_mm_set1_ps(x), _mm_set1_ps(y)).data;
        return ::atan2(x, y);
    }
    FF_INLINE float acos(const float x) {
        return ::acos(x);
    }
    FF_INLINE float cos(const float x) {
        return ::cos(x);
    }
    FF_INLINE float exp(const float x) {
        return ::exp(x);
    }
    FF_INLINE float sin(const float x) {
        return ::sin(x);
    }
    FF_INLINE float log(const float x) {
        return ::log(x);
    }
    FF_INLINE float tan(const float x) {
        return ::tan(x);
    }
    FF_INLINE float atan(const float x) {
        return ::atan(x);
    }

    // Gives approximate results!
    FF_INLINE float powFast(const float base, const float exponent) {
        return (float&)FastPow::fastPow(_mm_set1_ps(base), _mm_set1_ps(exponent));
    }
    FF_INLINE float pow(const float base, const float exponent) {
        return powf(base, exponent);
    }

    // Gives approximate results!
    FF_INLINE float exp2fast(const float x) {
        return (float&)FastPow::exp2f4(_mm_set1_ps(x));
    }

    // Gives approximate results!
    FF_INLINE float log2fast(const float x) {
        return (float&)FastPow::log2f4(_mm_set1_ps(x));
    }

    // Calculates logarithm of float in given base
    /// \param number number from which the logarithm is computed
    /// \param base base of the logarithm
    /// \return log_base(number)
    FF_INLINE float log(const float number, const float base) {
        return ::log(number)/::log(base);  
    }

    // Calculates int power for low exponents
    template<int TExp>
    FF_INLINE float pow(const float value);

    template<>
    FF_INLINE float pow<2>(const float value) {
        return value*value;
    }

    template<>
    FF_INLINE float pow<3>(const float value) {
        return value*value*value;
    }

    template<>
    FF_INLINE float pow<4>(const float value) {
        return pow<2>(pow<2>(value));
    }
    template<>
    FF_INLINE float pow<5>(const float value) {
        return pow<2>(pow<2>(value))*value;
    }
};

#pragma warning(pop)