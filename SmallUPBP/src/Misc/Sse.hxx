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

#ifndef __SSE_HXX__
#define __SSE_HXX__

#include <ostream>
#include <xmmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>

#include "..\PrecompiledLibs\FastFloat\FastFloat.h"
#include "Defs.hxx"

//////////////////////////////////////////////////////////////////////////
// Classes for fast computation with vectors using SSE.

namespace Sse {
    template<int a, int b, int c, int d>
    INLINE __m128 shuffle(const __m128 in) {
        return _mm_castsi128_ps(_mm_shuffle_epi32(_mm_castps_si128(in), _MM_SHUFFLE(d, c, b, a)));
    }
    template<int a, int b, int c, int d>
    INLINE __m128i shuffle(const __m128i in) {
        return _mm_shuffle_epi32(in, _MM_SHUFFLE(d, c, b, a));
    }

    // Extracts one value from a SSE 4-vector.
    INLINE float get(const __m128 in, const int index) {
        UPBP_ASSERT(unsigned(index) < 4);
        return ((float*)(&in))[index];
    }

	// Extracts reference on one value in a SSE 4-vector.
    INLINE float& getRef(__m128& in, const int index) {
        UPBP_ASSERT(unsigned(index) < 4);
        return ((float*)(&in))[index];
    }

	// Extracts one value from a SSE 8-vector.
    INLINE float get(const __m256& in, const int index) {
        UPBP_ASSERT(unsigned(index) < 8);
        return ((float*)(&in))[index];
    }

	// Extracts reference on one value in a SSE 8-vector.
    INLINE float& getRef(__m256& in, const int index) {
        UPBP_ASSERT(unsigned(index) < 8);
        return ((float*)(&in))[index];
    }
}


class Int4;
class Float4;

// Holds 4 bool values in a SSE vector. Is used for blend operations (mixing 2 SSE vectors based on a Bool4 mask).
class Bool4 {
protected:

    __declspec(align(16)) union {
        __m128i _sse;
        int _data[4];
    };

    INLINE bool isValid() const {
        for(int i = 0; i < 4; i++) {
            if((_data[i] != 0 && _data[i] != -1)) {
                return false;
            }
        }
        return true;
    } 

public:

    INLINE explicit Bool4(const __m128i _sse) : _sse(_sse) { }

    INLINE explicit Bool4(const __m128 _sse)  : _sse(_mm_castps_si128(_sse)) {
        UPBP_ASSERT(isValid());
    }

    // Initializes all 4 values to the same bool.
    INLINE explicit Bool4(const bool value) : _sse(_mm_set1_epi32(-1*value)) {
        UPBP_ASSERT(isValid());
    }

    INLINE const bool operator[](const int index) const {
        UPBP_ASSERT(unsigned(index) <= 3 && isValid());
        return this->_data[index] != 0;
    }

    INLINE Bool4 operator&&(const Bool4& other) const {
        return Bool4(_mm_and_si128(_sse, other._sse));
    }

    INLINE Bool4 operator==(const Bool4& other) const {
        return Bool4(_mm_cmpeq_epi32(_sse, other._sse));
    }

    INLINE bool allFalse() const {
        UPBP_ASSERT(isValid());
        return _mm_movemask_ps(_mm_castsi128_ps(_sse)) == 0;
    }

    INLINE bool allTrue() const {
        UPBP_ASSERT(isValid());
        return _mm_movemask_ps(_mm_castsi128_ps(_sse)) == 0xF;
    }

    INLINE bool anyFalse() const {
        return !allTrue();
    }

    // Returns all ones for true, all zeros for false elements.
    INLINE Int4 maskedInts(const uint mask) const;
    
    // Returns Float4 with trueVals for true elements and falseVales for false elements of this Bool4.
    INLINE Float4 blend(const Float4& trueVals, const Float4& falseVals) const;

    INLINE Int4 blend(const Int4& trueVals, const Int4& falseVals) const;
};

// 4 floating point values packed in a SSE vector. CPU is able to do most of the operations with them with the same cost as when manipulating single value.
class Float4 {    
public:
    __m128 data;
    
    INLINE Float4() { }

    INLINE explicit Float4(const __m128 _sse)                                 : data(_sse) { }
    
    INLINE Float4(const float x, const float y, const float z, const float w) : data(_mm_set_ps(w, z, y, x)) { }
    
    // Initializes all 4 values to the same float.
    INLINE explicit Float4(const float value)                                 : data(_mm_set1_ps(value)) { }
    
    // Loads an array of 4 floats into memory. MemoryLocation have to be aligned to 16B.
    INLINE Float4(const float* memoryLocation)                                : data(_mm_load_ps(memoryLocation)) {
        UPBP_ASSERT(((int64)memoryLocation%16) == 0);
    }

    INLINE const float operator[](const int index) const {
       return Sse::get(data, index);
    }

    INLINE float& operator[](const int index) {
       return Sse::getRef(data, index);
    }
    
    INLINE Float4 operator+(const Float4& other) const {
        return Float4(_mm_add_ps(data, other.data));
    }
    
    INLINE Float4 operator-(const Float4& other) const {
        return Float4(_mm_sub_ps(data, other.data));
    }
    
    INLINE Float4 operator*(const Float4& other) const {
        return Float4(_mm_mul_ps(data, other.data));
    }
    
    INLINE Float4 operator/(const Float4& other) const {
        return Float4(_mm_div_ps(data, other.data));
    }
    
    INLINE Float4 operator+(const float fact) const {
        return Float4(_mm_add_ps(data, _mm_set1_ps(fact)));
    }
    
    INLINE Float4 operator-(const float fact) const {
        return Float4(_mm_sub_ps(data, _mm_set1_ps(fact)));
    }
    
    INLINE Float4 operator*(const float fact) const {
        return Float4(_mm_mul_ps(data, _mm_set1_ps(fact)));
    }
    
    INLINE Float4 operator/(const float fact) const {
        UPBP_ASSERT(fact != 0.f);
        return Float4(_mm_div_ps(data, _mm_set1_ps(fact)));
    }
    
    friend INLINE Float4 operator+(const float fact, const Float4& vect) {
        return Float4(_mm_add_ps(_mm_set1_ps(fact), vect.data));
    }
    
    friend INLINE Float4 operator-(const float fact, const Float4& vect) {
        return Float4(_mm_sub_ps(_mm_set1_ps(fact), vect.data));
    }
    
    friend INLINE Float4 operator*(const float fact, const Float4& vect) {
        return Float4(_mm_mul_ps(_mm_set1_ps(fact), vect.data));
    }

    INLINE Float4& operator+=(const Float4& other) {
        this->data = _mm_add_ps(data, other.data);
        return *this;
    }

    INLINE Float4& operator-=(const Float4& other) {
        this->data = _mm_sub_ps(data, other.data);
        return *this;
    }

    INLINE Float4& operator*=(const Float4& other) {
        this->data = _mm_mul_ps(data, other.data);
        return *this;
    }

    INLINE Float4& operator/=(const Float4& other) {
        this->data = _mm_div_ps(data, other.data);
        return *this;
    }

    INLINE Float4& operator+=(const float other) {
        this->data = _mm_add_ps(data, _mm_set1_ps(other));
        return *this;
    }

    INLINE Float4 operator-() const {
        return Float4(_mm_xor_ps(data, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))));
    }

    // Returns absolute values.
    INLINE Float4 abs() const {
        return Float4(_mm_and_ps(data, _mm_castsi128_ps(_mm_set1_epi32(0x7fFFffFF))));
    }

    INLINE Float4& operator*=(const float factor) {
        this->data = _mm_mul_ps(data, _mm_set1_ps(factor));
        return *this;
    }

    INLINE Float4& operator/=(const float factor) {
        UPBP_ASSERT(factor != 0.f);
        this->data = _mm_div_ps(data, _mm_set1_ps(factor));
        return *this;
    }

    // Returns 1/values.
    INLINE Float4 getInverse() const {
        return Float4(_mm_div_ps(_mm_set1_ps(1.f), data));
    }

    // Tests if all values are not NaNs/INFs.
    INLINE bool isReal() const {
        return (((*this) < FLOAT_INFINITY) && ((*this) > -FLOAT_INFINITY) && Bool4(_mm_cmpeq_ps(data, data))).allTrue();
    }

    // Returns pairwise minimums from 2 vectors.
    static INLINE Float4 min(const Float4& v1, const Float4& v2) {
        return Float4(_mm_min_ps(v1.data, v2.data));
    }

	// Returns pairwise maximums from 2 vectors.
    static INLINE Float4 max(const Float4& v1, const Float4& v2) {
        return Float4(_mm_max_ps(v1.data, v2.data));
    }

    INLINE Bool4 operator<(const Float4& other) const {
        return Bool4(_mm_cmplt_ps(data, other.data));
    }

    INLINE Bool4 operator>(const Float4& other) const {
        return Bool4(_mm_cmpgt_ps(data, other.data));
    }

    INLINE Bool4 operator<=(const Float4& other) const {
        return Bool4(_mm_cmple_ps(data, other.data));
    }

    INLINE Bool4 operator>=(const Float4& other) const {
        return Bool4(_mm_cmpge_ps(data, other.data));
    }

    INLINE Bool4 operator<(const float other) const {
        return Bool4(_mm_cmplt_ps(data, _mm_set1_ps(other)));
    }

    INLINE Bool4 operator>(const float other) const {
        return Bool4(_mm_cmpgt_ps(data, _mm_set1_ps(other)));
    }

    INLINE Bool4 operator<=(const float other) const {
        return Bool4(_mm_cmple_ps(data, _mm_set1_ps(other)));
    }

    INLINE Bool4 operator>=(const float other) const {
        return Bool4(_mm_cmpge_ps(data, _mm_set1_ps(other)));
    }
    
    INLINE Bool4 operator==(const Float4& other) const {
        return Bool4(_mm_cmpeq_ps(data, other.data));
    }

    INLINE Float4 fastPow(const float exponent) const {
        return Float4(Ff::powFast(data, _mm_set1_ps(exponent)));
    }

    INLINE Float4 log() const {
        return Float4(Ff::log(data));
    }

    INLINE Float4 sqrtFast() const {
        return Float4(_mm_rcp_ps(_mm_rsqrt_ps(data)));
    }

    INLINE Float4 sqrt() const {
        return Float4(_mm_sqrt_ps(data));
    }

    INLINE Float4 invSqrt() const {
        return sqrt().getInverse();
    }

    INLINE Float4 invSqrtFast() const {
        return Float4(_mm_rsqrt_ps(data));
    }

    // Returns average of the 4 values stored.
    INLINE float avg() const {
        return ((Sse::get(data, 0) + Sse::get(data, 1)) + (Sse::get(data, 2) + Sse::get(data, 3)))*0.25f;
    }    

    // Returns dot product of 2 4-element vectors.
    static INLINE float dot(const Float4& v1, const Float4& v2) {
#ifndef LEGACY_CPU
        return _mm_cvtss_f32(_mm_dp_ps(v1.data, v2.data, 0xF1));
#else
        const Float4 res = v1*v2;
        return res[0]+res[1]+res[2]+res[3];
#endif
    }

    // Returns dot product of this vector with itself.
    INLINE float square() const {
        return dot(*this, *this);
    }

	INLINE float l2normFast() const {
		return Ff::sqrtFast(dot(*this, *this));
	}

    INLINE float l2norm() const {
        return Ff::sqrt(dot(*this, *this));
    }

    // Clamps this vectors values to lie between minimum and maximum.
    INLINE Float4 clamp(const float minimum, const float maximum) const {
        return Float4(_mm_min_ps(_mm_max_ps(_mm_set1_ps(minimum), data), _mm_set1_ps(maximum)));
    }

    // epsilon-insensitive comparison.
    static INLINE Bool4 equal(const Float4& x, const Float4& y, const float epsilon) {
        return Bool4((x-y).abs() < epsilon);
    }    
};

INLINE std::ostream& operator<<(std::ostream& os, const Float4& vect) {
    os << vect[0] << ' ' << vect[1] <<  ' ' << vect[2] << ' ' << vect[3];
    return os;
}

INLINE std::istream& operator>>(std::istream& is, Float4& vect) {
    is >> vect[0] >> vect[1] >> vect[2] >> vect[3];
    return is;
}

// Vector of 4 int values.
class Int4 {
public:
    __declspec(align(16)) union {
        __m128i _sse;

        struct { 
            int x;
            int y;
            int z;
            int w;
        };

        int _data[4];
    };

    INLINE Int4() { };

    INLINE explicit Int4(const __m128i _sse) : _sse(_sse) { }
    
    INLINE explicit Int4(const int value) : _sse(_mm_set1_epi32(value)) { }
    
    INLINE Int4(const int x, const int y, const int z, const int w) : _sse(_mm_set_epi32(w, z, y, x)) { }
    
    INLINE explicit Int4(const int* data) : _sse(_mm_castps_si128(_mm_load_ps((const float*)data))) {
        UPBP_ASSERT(((int64)data%16) == 0);
    }

    // Reinterprets a chunk of memory as Int4 vector (no conversion takes place).
    static INLINE Int4 reinterpretFloat(const Float4& input) {
        return Int4(_mm_castps_si128(input.data));
    }

    INLINE const int operator[](const int index) const {
        UPBP_ASSERT(unsigned(index) <= 3);
        return this->_data[index];
    }

    INLINE int& operator[](const int index) {
        UPBP_ASSERT(unsigned(index) <= 3);
        return this->_data[index];
    }

    INLINE Int4 operator+(const Int4& other) const {
        return Int4(_mm_add_epi32(_sse, other._sse));
    }

    INLINE Int4 operator-(const Int4& other) const {
        return Int4(_mm_sub_epi32(_sse, other._sse));
    }

    INLINE Int4 operator*(const Int4& other) const {
        return Int4(_mm_mul_epi32(_sse, other._sse));
    }

    INLINE Int4 operator+(const int fact) const {
        return Int4(_mm_add_epi32(_sse, _mm_set1_epi32(fact)));
    }

    INLINE Int4 operator-(const int fact) const {
        return Int4(_mm_sub_epi32(_sse, _mm_set1_epi32(fact)));
    }

    INLINE Int4 operator*(const int fact) const {
#ifdef LEGACY_CPU
        return Int4(_mm_set_epi32((*this)[0]*fact, (*this)[1]*fact, (*this)[2]*fact, (*this)[3]));
#else
        return Int4(_mm_mul_epi32(_sse, _mm_set1_epi32(fact)));
#endif
    }

    INLINE Int4& operator+=(const Int4& other) {
        this->_sse = _mm_add_epi32(_sse, other._sse);
        return *this;
    }

    INLINE Int4& operator-=(const Int4& other) {
        _sse = _mm_sub_epi32(_sse, other._sse);
        return *this;
    }

    static INLINE Int4 min(const Int4& x, const Int4& y) {
#ifdef LEGACY_CPU
        Int4 res = x;
        for(int i = 0; i < 4; i++) {
            if(res[i] > y[i]) {
                res[i] = y[i];
            }
        }
        return res;
#else
        return Int4(_mm_min_epi32(x._sse, y._sse));
#endif
    }
    static INLINE Int4 max(const Int4& x, const Int4& y) {
#ifdef LEGACY_CPU
        Int4 res = x;
        for(int i = 0; i < 4; i++) {
            if(res[i] < y[i]) {
                res[i] = y[i];
            }
        }
        return res;
#else
        return Int4(_mm_max_epi32(x._sse, y._sse));
#endif
    }
    static INLINE Int4 clamp(const Int4& value, const Int4& minimum, const Int4& maximum) {
        return Int4::max(Int4::min(value, maximum), minimum);
    }

    INLINE int min() const {
#ifdef LEGACY_CPU
        int res = (*this)[0];
        for(int i = 1; i < 4; i++) {
            if((*this)[i] < res) {
                res = (*this)[i];
            }
        }
        return res;
#else
        const __m128i h = _mm_min_epi32(Sse::shuffle<1,0,3,2>(_sse), _sse);
        const int res = _mm_extract_epi32(_mm_min_epi32(Sse::shuffle<2,3,0,1>(h), h), 1);
#endif
        return res;
    }

    
    // Converts the values to floating point.
    INLINE Float4 toFloat() const {
        return Float4(_mm_cvtepi32_ps(_sse));
    }

    // Creates an Int vector from Float one with conversion of values.
    static INLINE Int4 float2int(const Float4& data) {
        return Int4(_mm_cvtps_epi32(data.data));
    }
};

INLINE Int4 Bool4::maskedInts(const uint mask) const {
    return Int4(_mm_and_si128(_sse, _mm_set1_epi32(mask)));
}

INLINE Float4 Bool4::blend(const Float4& trueVals, const Float4& falseVals) const {
#ifdef LEGACY_CPU
    Float4 res = trueVals;
    for(int i = 0; i < 4; i++) {
        if((*this)[i] == false) {
            res[i] = falseVals[i];
        }
    }
    return res;
#else
    return Float4(_mm_blendv_ps(falseVals.data, trueVals.data, _mm_castsi128_ps(this->_sse)));
#endif
}

INLINE Int4 Bool4::blend(const Int4& trueVals, const Int4& falseVals) const {
#ifdef LEGACY_CPU
    Int4 res = trueVals;
    for(int i = 0; i < 4; i++) {
        if((*this)[i] == false) {
            res[i] = falseVals[i];
        }
    }
    return res;
#else
    return Int4(_mm_castps_si128(_mm_blendv_ps(_mm_castsi128_ps(falseVals._sse), _mm_castsi128_ps(trueVals._sse), _mm_castsi128_ps(this->_sse))));
#endif
}


#if 0
//AVX stuff

class Float8 {    
public:
    __m256 data;
    
	INLINE Float8() { }
   
    INLINE explicit Float8(const __m256& data)                                 : data(data) { }
    //INLINE Float8(const float x, const float y, const float z, const float w) : data(_mm256_set_ps(w, z, y, x)) { }
    INLINE explicit Float8(const float value)                                 : data(_mm256_set1_ps(value)) { }
    INLINE Float8(const float* memoryLocation)                                : data(_mm256_load_ps(memoryLocation)) {
        UPBP_ASSERT(((int64)memoryLocation%32) == 0);
    }

    INLINE const float operator[](const int index) const {
        return Sse::get(data, index);
    }
    INLINE float& operator[](const int index) {
        return Sse::getRef(data, index);
    }
    INLINE Float8 operator+(const Float8& other) const {
        return Float8(_mm256_add_ps(data, other.data));
    }
    INLINE Float8 operator-(const Float8& other) const {
        return Float8(_mm256_sub_ps(data, other.data));
    }
    INLINE Float8 operator*(const Float8& other) const {
        return Float8(_mm256_mul_ps(data, other.data));
    }
    INLINE Float8 operator/(const Float8& other) const {
        return Float8(_mm256_div_ps(data, other.data));
    }
    INLINE Float8 operator+(const float fact) const {
        return Float8(_mm256_add_ps(data, _mm256_set1_ps(fact)));
    }
    INLINE Float8 operator-(const float fact) const {
        return Float8(_mm256_sub_ps(data, _mm256_set1_ps(fact)));
    }
    INLINE Float8 operator*(const float fact) const {
        return Float8(_mm256_mul_ps(data, _mm256_set1_ps(fact)));
    }
    INLINE Float8 operator/(const float fact) const {
        UPBP_ASSERT(fact != 0.f);
        return Float8(_mm256_div_ps(data, _mm256_set1_ps(fact)));
    }
    friend INLINE Float8 operator+(const float fact, const Float8& vect) {
        return Float8(_mm256_add_ps(_mm256_set1_ps(fact), vect.data));
    }
    friend INLINE Float8 operator-(const float fact, const Float8& vect) {
        return Float8(_mm256_sub_ps(_mm256_set1_ps(fact), vect.data));
    }
    friend INLINE Float8 operator*(const float fact, const Float8& vect) {
        return Float8(_mm256_mul_ps(_mm256_set1_ps(fact), vect.data));
    }

    INLINE Float8& operator+=(const Float8& other) {
        this->data = _mm256_add_ps(data, other.data);
        return *this;
    }
    INLINE Float8& operator-=(const Float8& other) {
        this->data = _mm256_sub_ps(data, other.data);
        return *this;
    }
    INLINE Float8& operator*=(const Float8& other) {
        this->data = _mm256_mul_ps(data, other.data);
        return *this;
    }
    INLINE Float8& operator/=(const Float8& other) {
        this->data = _mm256_div_ps(data, other.data);
        return *this;
    }
    INLINE Float8& operator+=(const float other) {
        this->data = _mm256_add_ps(data, _mm256_set1_ps(other));
        return *this;
    }
    INLINE Float8 operator-() const {
        return Float8(_mm256_xor_ps(data, _mm256_castsi256_ps(_mm256_set1_epi32(0x80000000))));
    }
    INLINE Float8 abs() const {
        return Float8(_mm256_and_ps(data, _mm256_castsi256_ps(_mm256_set1_epi32(0x7fFFffFF))));
    }    
    INLINE Float8& operator*=(const float factor) {
        this->data = _mm256_mul_ps(data, _mm256_set1_ps(factor));
        return *this;
    }
    INLINE Float8& operator/=(const float factor) {
        UPBP_ASSERT(factor != 0.f);
        this->data = _mm256_div_ps(data, _mm256_set1_ps(factor));
        return *this;
    }

    INLINE Float8 getInverse() const {
        return Float8(_mm256_div_ps(_mm256_set1_ps(1.f), data));
    }
    INLINE bool isReal() const {
        return (_mm256_movemask_ps(_mm256_cmp_ps(data, _mm256_set1_ps(FLOAT_INFINITY), _CMP_LT_OQ)) &
            _mm256_movemask_ps(_mm256_cmp_ps(data, _mm256_set1_ps(-FLOAT_INFINITY), _CMP_GT_OQ)) &
            _mm256_movemask_ps(_mm256_cmp_ps(data, data, _CMP_EQ_UQ))) == 255;
    }

    static INLINE Float8 min(const Float8& v1, const Float8& v2) {
        return Float8(_mm256_min_ps(v1.data, v2.data));
    }
    static INLINE Float8 max(const Float8& v1, const Float8& v2) {
        return Float8(_mm256_max_ps(v1.data, v2.data));
    }

    INLINE Float8 sqrtFast() const {
        return Float8(_mm256_rcp_ps(_mm256_rsqrt_ps(data)));
    }
    INLINE Float8 sqrt() const {
        return Float8(_mm256_sqrt_ps(data));
    }
    INLINE Float8 invSqrt() const {
        return sqrt().getInverse();
    }
    INLINE Float8 invSqrtFast() const {
        return Float8(_mm256_rsqrt_ps(data));
    }
    INLINE Float8 clamp(const float minimum, const float maximum) const {
        return Float8(_mm256_min_ps(_mm256_max_ps(_mm256_set1_ps(minimum), data), _mm256_set1_ps(maximum)));
    }  
};

#endif

#endif //__SSE_HXX__