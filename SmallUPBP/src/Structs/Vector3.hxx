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

#ifndef __VECTOR3_HXX__
#define __VECTOR3_HXX__

#include <xmmintrin.h>
#include <smmintrin.h>

#include "..\Misc\Utils.hxx"

//////////////////////////////////////////////////////////////////////////
// Classes representing position and direction

class Dir;

// Position in 3D space. Uses SSE vector with 1 extra component, that can be accessed via extraData methods. The extra value is not preserved when Pos is changed!
class Pos {
protected:

public:
    __m128 data;

    typedef Dir DirType;

    static const int DIMENSION = 3;
    
    // (0, 0, 0)
    static const Pos ZERO;

    // (1, 1, 1)
    static const Pos ONES;

    // (1, 0, 0)
    static const Pos UNIT_X;

    // (0, 1, 0)
    static const Pos UNIT_Y;

    // (0, 0, 1)
    static const Pos UNIT_Z;

    // (-1, -1, -1)
    static const Pos NEGATIVE_ONES;

    INLINE Pos() { }

    INLINE explicit Pos(const __m128& _sse) : data(_sse) { }

    INLINE Pos(const float x, const float y, const float z) {
        this->x() = x;
        this->y() = y;
        this->z() = z;
    }

	INLINE Pos(const float x, const float y, const float z, const float extra) {
		this->x() = x;
		this->y() = y;
		this->z() = z;
		this->extraData() = extra;
	}

    // Sets all components to the same value.
    INLINE explicit Pos(const float x) : data(_mm_set1_ps(x)) { }

    // Sets value of this vector and preserves the extra data stored
    INLINE void setAndPreserveExtra(const Pos& other) {
        this->x() = other.x();
        this->y() = other.y();
        this->z() = other.z();
    }

	// Sets value of this vector and preserves the extra data stored.
	INLINE void set(const float x, const float y, const float z) {
		this->x() = x;
		this->y() = y;
		this->z() = z;
	}

    INLINE const float x() const {
        return (*this)[0];
    }
    INLINE float& x() {
        return (*this)[0];
    }
    INLINE const float y() const {
        return (*this)[1];
    }
    INLINE float& y() {
        return (*this)[1];
    }
    INLINE const float z() const {
        return (*this)[2];
    }
    INLINE float& z() {
        return (*this)[2];
    }

    // Returns the extra data stored as float.
    INLINE float extraData() const {
        return Sse::get(data, 3);
    }

    // Returns reference to the extra data stored as float.
    INLINE float& extraData() {
        return Sse::getRef(data, 3);
    }

    // Returns the extra data stored as int.
    INLINE int extraDataInt() const {
        const float temp = Sse::get(data, 3);
        return (int&)temp;
    }

    // Returns reference to the extra data stored as int.
    INLINE int& extraDataInt() {
        return (int&)Sse::getRef(data, 3);
    }

    INLINE const float operator[](const int index) const {
        UPBP_ASSERT(unsigned(index) < 3);
        return Sse::get(data, index);
    }
    INLINE float& operator[](const int index) {
        UPBP_ASSERT(unsigned(index) < 3);
        return Sse::getRef(data, index);
    }

   
    INLINE Pos operator+(const Pos& other) const {
        return Pos(_mm_add_ps(data, other.data));
    }

    INLINE Pos operator-() const {
        return Pos(_mm_xor_ps(data, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))));
    }
    
    INLINE Pos abs() const {
        return Pos(_mm_and_ps(data, _mm_castsi128_ps(_mm_set1_epi32(0x7fFFffFF))));
    }
    
    INLINE Pos operator*(const float factor) const {
        return Pos(_mm_mul_ps(data, _mm_set1_ps(factor)));
    }
    
    INLINE Pos operator/(const float factor) const {
        UPBP_ASSERT(factor != 0.f);
        return Pos(_mm_div_ps(data, _mm_set1_ps(factor)));
    }
    
    INLINE Pos& operator*=(const float factor) {
        this->data = _mm_mul_ps(data, _mm_set1_ps(factor));
        return *this;
    }
    
    INLINE Pos& operator/=(const float factor) {
        UPBP_ASSERT(factor != 0.f);
        this->data = _mm_div_ps(data, _mm_set1_ps(factor));
        return *this;
    }
    
    friend INLINE Pos operator*(const float factor, const Pos& vector) {
        return Pos(_mm_mul_ps(vector.data, _mm_set1_ps(factor)));
    }
    
    friend INLINE Pos operator/(const float factor, const Pos& vector) {
        UPBP_ASSERT(vector.x() != 0.f && vector.y() != 0.f && vector.z() != 0.f);
        return Pos(_mm_div_ps(_mm_set1_ps(factor), vector.data));
    }

    INLINE float min() const {
        return Utils::min(x(), y(), z());
    }
    
    INLINE float max() const {
        return Utils::max(x(), y(), z());
    }    
    
    // Returns index of the smallest element.
    INLINE int argMin() const {
        return (x() < y()) ? ((x() < z()) ? 0 : 2) : ((y() < z()) ? 1 : 2);
    }
    
    // Returns index of the largest element.
    INLINE int argMax() const {
        return (x() > y()) ? ((x() > z()) ? 0 : 2) : ((y() > z()) ? 1 : 2);
    }
    
    INLINE float l1Norm() const {
        const Pos absVals = this->abs();
        return absVals.x() + absVals.y() + absVals.z();
    }

    INLINE bool isReal() const {
        return (_mm_movemask_ps(_mm_and_ps(_mm_and_ps(
                    _mm_cmplt_ps(data, _mm_set1_ps(FLOAT_INFINITY)),
                    _mm_cmpgt_ps(data, _mm_set1_ps(-FLOAT_INFINITY))            
                ),
                    _mm_cmpeq_ps(data, data))) & 0x7) == 0x7;
    }
    
    INLINE bool operator==(const Pos& other) const {
        return (_mm_movemask_ps(_mm_cmpeq_ps(data, other.data)) & 0x7) == 0x7;
    }
    INLINE bool operator!=(const Pos& other) const {
        return !(*this == other);
    }

    static INLINE Pos min(const Pos& v1, const Pos& v2) {
        return Pos(_mm_min_ps(v1.data, v2.data));
    }
    static INLINE Pos max(const Pos& v1, const Pos& v2) {
        return Pos(_mm_max_ps(v1.data, v2.data));
    }

    INLINE Pos clamp(const Pos& low, const Pos& high) const {
        UPBP_ASSERT(low.x() <= high.x() && low.y() <= high.y() && low.z() < high.z());
        return Pos::min(Pos::max(*this, low), high);
    }
};


// Direction in 3D space. Uses SSE vector with 1 extra component, that can be accessed via extraData methods. The extra value is not preserved when Pos is changed!
class Dir {
protected:

public:
    __m128 data;

    static const int DIMENSION = 3;
    
    // (0, 0, 0)
    static const Dir ZERO;

    // (1, 1, 1)
    static const Dir ONES;

    // (1, 0, 0)
    static const Dir UNIT_X;

    // (0, 1, 0)
    static const Dir UNIT_Y;

    // (0, 0, 1)
    static const Dir UNIT_Z;

    // (-1, -1, -1)
    static const Dir NEGATIVE_ONES;

    INLINE Dir() {}

    INLINE explicit Dir(const __m128& _sse) : data(_sse) { }

    INLINE Dir(const float x, const float y, const float z) {
        this->x() = x;
        this->y() = y;
        this->z() = z;
    }

	INLINE Dir(const float x, const float y, const float z, const float extra) {
		this->x() = x;
		this->y() = y;
		this->z() = z;
		this->extraData() = extra;
	}

    // Sets all components to the same value.
    INLINE explicit Dir(const float x) : data(_mm_set1_ps(x)) { }

    INLINE const float x() const {
        return (*this)[0];
    }
    INLINE float& x() {
        return (*this)[0];
    }
    INLINE const float y() const {
        return (*this)[1];
    }
    INLINE float& y() {
        return (*this)[1];
    }
    INLINE const float z() const {
        return (*this)[2];
    }
    INLINE float& z() {
        return (*this)[2];
    }

    // Sets this object value without modifying the extra data stored.
    INLINE void setAndPreserveExtra(const Dir& other) {
        this->x() = other.x();
        this->y() = other.y();
        this->z() = other.z();
    }

    // Returns the extra data stored as float.
    INLINE float extraData() const {
        return Sse::get(data, 3);
    }

    // Returns the extra data stored as float reference.
    INLINE float& extraData() {
        return Sse::getRef(data, 3);
    }

    // Returns the extra data stored as int.
    INLINE int extraDataInt() const {
        const float temp = Sse::get(data, 3);
        return (int&)temp;
    }

    // Returns the extra data stored as int reference.
    INLINE int& extraDataInt() {
        return (int&)Sse::getRef(data, 3);
    }

    INLINE const float operator[](const int index) const {
        UPBP_ASSERT(unsigned(index) < 3);
        return Sse::get(data, index);
    }

    INLINE float& operator[](const int index) {
        UPBP_ASSERT(unsigned(index) < 3);
        return Sse::getRef(data, index);
    }

    INLINE float sizeApprox() const {
        return Ff::sqrtFast(square());
    }

    INLINE float size() const {
        return Ff::sqrt(square());
    }

    INLINE float square() const {
        const float ret = dot(*this, *this);
        UPBP_ASSERT(Float::isReal(ret));
        return ret;
    }

    INLINE Dir getNormalizedApprox() const {
        return Dir(_mm_mul_ps(data, _mm_set1_ps(Ff::invSqrtFast(square()))));
    }

    INLINE Dir getNormalized() const {
        return Dir(_mm_mul_ps(data, _mm_set1_ps(Ff::invSqrt(square()))));
    }

    INLINE Dir getNormalized(float& originalSize) const {
        originalSize = size();
        return *this/originalSize;
    }

    INLINE Dir getNormalizedApprox(float& originalSize) const {
        originalSize = sizeApprox();
        return *this/originalSize;
    }

    INLINE bool isNormalized() const {
        return std::abs(this->square() - 1.f) < 1e-4f;
    }

    INLINE bool isRoughlyNormalized() const {
        return std::abs(this->square() - 1.f) < 0.01f;
    }

    INLINE Dir operator+(const Dir& other) const {
        return Dir(_mm_add_ps(data, other.data));
    }

    INLINE Dir& operator+=(const Dir& other) {
        this->data = _mm_add_ps(data, other.data);
        return *this;
    }

    INLINE Dir& operator-=(const Dir& other) {
        this->data = _mm_sub_ps(data, other.data);
        return *this;
    }

    INLINE Dir operator-(const Dir& other) const {
        return Dir(_mm_sub_ps(data, other.data));
    }

    INLINE Dir operator/(const Dir& other) const {
        UPBP_ASSERT(other.x() != 0.f && other.y() != 0.f && other.z() != 0.f);
        return Dir(_mm_div_ps(data, other.data));
    }

    INLINE Dir& operator*=(const Dir& other) {
        this->data = _mm_mul_ps(data, other.data);
        return *this;
    }

    INLINE Dir& operator/=(const Dir& other) {
        UPBP_ASSERT(other.x() != 0.f && other.y() != 0.f && other.z() != 0.f);
        this->data = _mm_div_ps(data, other.data);
        return *this;
    }

    INLINE Dir operator-() const {
        return Dir(_mm_xor_ps(data, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))));
    }

    INLINE Dir abs() const {
        return Dir(_mm_and_ps(data, _mm_castsi128_ps(_mm_set1_epi32(0x7fFFffFF))));
    }

    INLINE Dir operator*(const float factor) const {
        return Dir(_mm_mul_ps(data, _mm_set1_ps(factor)));
    }

    INLINE Dir operator*(const Dir& other) const {
        return Dir(_mm_mul_ps(data, other.data));
    }

    INLINE Dir operator/(const float factor) const {
        UPBP_ASSERT(factor != 0.f);
        return Dir(_mm_div_ps(data, _mm_set1_ps(factor)));
    }

    INLINE Dir& operator*=(const float factor) {
        this->data = _mm_mul_ps(data, _mm_set1_ps(factor));
        return *this;
    }

    INLINE Dir& operator/=(const float factor) {
        UPBP_ASSERT(factor != 0.f);
        this->data = _mm_div_ps(data, _mm_set1_ps(factor));
        return *this;
    }

    friend INLINE Dir operator*(const float factor, const Dir& vector) {
        return Dir(_mm_mul_ps(vector.data, _mm_set1_ps(factor)));
    }

    friend INLINE Dir operator/(const float factor, const Dir& vector) {
        //UPBP_ASSERT(vector.x() != 0.f && vector.y() != 0.f && vector.z() != 0.f);
        return Dir(_mm_div_ps(_mm_set1_ps(factor), vector.data));
    }

    INLINE float min() const {
        return Utils::min(x(), y(), z());
    }

    INLINE float max() const {
        return Utils::max(x(), y(), z());
    }    

    INLINE int argMin() const {
        return (x() < y()) ? ((x() < z()) ? 0 : 2) : ((y() < z()) ? 1 : 2);
    }

    INLINE int argMax() const {
        return (x() > y()) ? ((x() > z()) ? 0 : 2) : ((y() > z()) ? 1 : 2);
    }

    INLINE float l1Norm() const {
        const Dir absVals = this->abs();
        return absVals.x() + absVals.y() + absVals.z();
    }

    INLINE float avg() const {
        return (x()+y()+z())/3.f;
    }

    INLINE Dir sqrt() const {
        return Dir(Ff::sqrt(data));
    }

    INLINE Dir getInverse() const {
        return Dir(_mm_div_ps(_mm_set1_ps(1.f), data));
    }

    INLINE bool isReal() const {
        return (_mm_movemask_ps(_mm_and_ps(_mm_and_ps(
                    _mm_cmplt_ps(data, _mm_set1_ps(FLOAT_INFINITY)),
                    _mm_cmpgt_ps(data, _mm_set1_ps(-FLOAT_INFINITY))            
                ),
                    _mm_cmpeq_ps(data, data))) & 0x7) == 0x7;
    }
    
    INLINE bool operator==(const Dir& other) const {
        return (_mm_movemask_ps(_mm_cmpeq_ps(data, other.data)) & 0x7) == 0x7;
    }
    INLINE bool operator!=(const Dir& other) const {
        return !(*this == other);
    }

    friend INLINE Dir cross(const Dir& v1, const Dir& v2) {
        return Dir(_mm_sub_ps(_mm_mul_ps(Sse::shuffle<1,2,0,3>(v1.data), Sse::shuffle<2,0,1,3>(v2.data)), 
            _mm_mul_ps(Sse::shuffle<2,0,1,3>(v1.data), Sse::shuffle<1,2,0,3>(v2.data))));
    }

    friend INLINE float dot(const Dir& v1, const Dir& v2) {
#ifndef LEGACY_CPU
        return _mm_cvtss_f32(_mm_dp_ps(v1.data, v2.data, 0x71));
#else
        const Dir res = v1*v2;
        return res[0]+res[1]+res[2];
        #pragma message ("Using legacy non-SSE4.1 code")
#endif
    }

    friend INLINE float dot(const Pos& v1, const Dir& v2) {
#ifndef LEGACY_CPU
        return _mm_cvtss_f32(_mm_dp_ps(v1.data, v2.data, 0x71));
#else
        const Dir res = v1*v2;
        return res[0]+res[1]+res[2];
        #pragma message ("Using legacy non-SSE4.1 code")
#endif
    }

    friend INLINE float dot(const Dir& v1, const Pos& v2) {
#ifndef LEGACY_CPU
        return _mm_cvtss_f32(_mm_dp_ps(v1.data, v2.data, 0x71));
#else
        const Dir res = v1*v2;
        return res[0]+res[1]+res[2];
        #pragma message ("Using legacy non-SSE4.1 code")
#endif
    }


    friend INLINE float absDot(const Dir& v1, const Dir& v2) {
        return ::abs(dot(v1, v2));
    }

    static INLINE Dir min(const Dir& v1, const Dir& v2) {
        return Dir(_mm_min_ps(v1.data, v2.data));
    }
    static INLINE Dir max(const Dir& v1, const Dir& v2) {
        return Dir(_mm_max_ps(v1.data, v2.data));
    }
    
    INLINE Dir clamp(const Dir& low, const Dir& high) const {
        UPBP_ASSERT(low.x() <= high.x() && low.y() <= high.y() && low.z() < high.z());
        return Dir::min(Dir::max(*this, low), high);
    }
};


INLINE Pos operator+(const Pos& point, const Dir& dir) {
    return Pos(_mm_add_ps(point.data, dir.data));
}

INLINE Dir operator-(const Pos& point1, const Pos& point2) {
    return Dir(_mm_sub_ps(point1.data, point2.data));
}

INLINE Pos& operator+=(Pos& point, const Dir& dir) {
    point.data = _mm_add_ps(point.data, dir.data);
    return point;
}

// Returns distance between two points.
INLINE float dist(const Pos& v1, const Pos& v2) {
    return (v1-v2).size();
}

// Returns square of distance between two points.
INLINE float distSqr(const Pos& v1, const Pos& v2) {
    return (v1-v2).square();
}

// Returns approximate distance between two points.
INLINE float distApprox(const Pos& v1, const Pos& v2) {
    return (v1-v2).sizeApprox();
}

INLINE Pos operator-(const Pos& point, const Dir& direction) {
    return Pos(_mm_sub_ps(point.data, direction.data));
}
INLINE Pos& operator-=(Pos& pos, const Dir& other) {
    pos.data = _mm_sub_ps(pos.data, other.data);
    return pos;
}

// Converts position to direction.
INLINE Dir POS_TO_DIR(const Pos& pos) {
    return Dir(pos.data);
}

// Converts direction to position.
INLINE Pos DIR_TO_POS(const Dir& pos) {
    return Pos(pos.data);
}

#endif //__VECTOR3_HXX__