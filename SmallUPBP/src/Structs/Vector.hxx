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

#ifndef __VECTOR_HXX__
#define __VECTOR_HXX__

#include <iostream>

#include "Vector3.hxx"

// World coordinates axes. Used as array indices.
enum Axes : uint8 {
    AXIS_ENUM_X = 0,
    AXIS_ENUM_Y = 1,
    AXIS_ENUM_Z = 2,
};

// Helper for easily defining arbitrary dimension vectors.
template<class TVector>
class Vector {
protected:

    INLINE float get(const int axis) const {
        return reinterpret_cast<const TVector*>(this)->get(axis);
    }

    INLINE float& get(const int axis) {
        return reinterpret_cast<TVector*>(this)->get(axis);
    }

    INLINE Vector() { }

public:

    typedef Vector DirType;

    INLINE float operator[]( const int axis ) const {
        return get(axis);
    }

    INLINE float& operator[]( const int axis ) {
        return get(axis);
    }

    INLINE float max() const {
        float maximum = get(0);
        for(int i = 1; i < TVector::DIMENSION; ++i) {
            if(get(i) > maximum) {
                maximum = get(i);
            }
        }
        return maximum;
    }

    INLINE float min() const {
        float minimum = get(0);
        for(int i = 1; i < TVector::DIMENSION; ++i) {
            if(get(i) < minimum) {
                minimum = get(i);
            }
        }
        return minimum;
    }

    INLINE int argMax() const {
        int maximum = 0;
        for(int i = 1; i < TVector::DIMENSION; ++i) {
            if(get(i) > get(maximum)) {
                maximum = i;
            }
        }
        return maximum;
    }

    INLINE int argMin() const {
        int minimum = 0;
        for(int i = 1; i < TVector::DIMENSION; ++i) {
            if(get(i) < get(minimum)) {
                minimum = i;
            }
        }
        return minimum;
    }

    INLINE TVector& operator+=( const TVector other ) {
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            get(i) += other.get(i);
        }
        return *reinterpret_cast<TVector*>(this);
    }

    INLINE TVector& operator-=( const TVector other ) {
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            get(i) -= other.get(i);
        }
        return *reinterpret_cast<TVector*>(this);
    }

    INLINE TVector& operator*=( const TVector other ) {
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            get(i) *= other.get(i);
        }
        return *reinterpret_cast<TVector*>(this);
    }

    INLINE TVector& operator/=( const TVector other ) {
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            UPBP_ASSERT(other.get(i) != 0.f);
            get(i) /= other.get(i);
        }
        return *reinterpret_cast<TVector*>(this);
    }

    INLINE TVector& operator*=( const float factor ) {
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            get(i) *= factor;
        }
        return *reinterpret_cast<TVector*>(this);
    }

    INLINE TVector& operator/=( const float factor ) {
        UPBP_ASSERT(factor != 0.f);
        const float inv = 1.f / factor;
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            get(i) *= inv;
        }
        return *reinterpret_cast<TVector*>(this);
    }

    INLINE TVector operator+( const TVector other ) const {
        TVector result;
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            result.get(i) = get(i) + other.get(i);
        }
        return result;
    }

    INLINE TVector operator-( const TVector other ) const {
        TVector result;
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            result.get(i) = get(i) - other.get(i);
        }
        return result;
    }

    INLINE TVector operator*( const TVector other ) const{
        TVector result;
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            result.get(i) = get(i) * other.get(i);
        }
        return result;
    }

    INLINE TVector operator/( const TVector other ) const {
        TVector result;
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            UPBP_ASSERT(other.get(i) != 0.f);
            result.get(i) = get(i) / other.get(i);
        }
        return result;
    }

    INLINE TVector operator/( const float number ) const {
        UPBP_ASSERT(number != 0.f);
        const float inv = 1.f / number;
        TVector result;
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            result.get(i) = get(i) * inv;
        }
        return result;
    }

    INLINE TVector operator*( const float number ) const {
        TVector result;
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            result.get(i) = get(i) * number;
        }
        return result;
    }

    INLINE TVector operator-() const {
        TVector result;
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            result.get(i) = get(i) * -1.f;
        }
        return result;
    }

    INLINE float square() const {
        float result = 0.f;
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            result += get(i)*get(i);
        }
        UPBP_ASSERT(Float::isReal(result));
        UPBP_ASSERT(result >= 0.f);
        return result;
    }

    INLINE float l1Norm() const {
        float result = 0.f;
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            result += abs(get(i));
        }
        return result;
    }

    // Returns size of this vector (Euclidean norm).
    INLINE float sizeFast() const {
        return Ff::sqrtFast(square());
    }

    INLINE float size() const {
        return Ff::sqrt(square());
    }

    // Returns vector with same direction and approximately unit size.
    INLINE TVector getNormalizedFast() const {
        const float factor = 1.f / sizeFast();
        return *this * factor;
    }

    // Returns vector with same direction and unit size.
    INLINE TVector getNormalized() const {
        const float factor = 1.f / size();
        return *this * factor;
    }

    INLINE TVector getAbsValues() const {
        TVector result;
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            result.get(i) = abs(get(i));
        }
        return result;
    }

    // Returns vector with each component inverted (1/x).
    INLINE TVector getInverse() const {
        TVector result;
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            result.get(i) = 1.f / get(i);
        }
        return result;
    }

    // Tells if this vector is normalized, e.g. has size of 1 (or very close number).
    INLINE bool isNormalized() const {
        return abs(this->square() - 1.f) < 1e-5;
    }

    // Returns true if each component of this vector is not NaN nor positive/negative infinity.
    INLINE bool isReal() const {
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            if(!Float::isReal(get(i))) {
                return false;
            }
        }
        return true;
    }

    // Returns vector with smaller values from both input vectors in every component.
    static INLINE TVector min( const TVector first, const TVector second ) {
        TVector result;
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            result.get(i) = Utils::min(first.get(i), second.get(i));
        }
        return result;
    }

    // Returns vector with bigger values from both input vectors in every component.
    static INLINE TVector max( const TVector first, const TVector second ) {    
        TVector result;
        for(int i = 0; i < TVector::DIMENSION; ++i) {
            result.get(i) = Utils::max(first.get(i), second.get(i));
        }
        return result;
    }
};

template<class TVector>
static INLINE float dot( const TVector v1, const TVector v2 ) {
    
    float result = 0;
    for(int i = 0; i < TVector::DIMENSION; ++i) {
        result += v1.get(i)*v2.get(i);
    }
    UPBP_ASSERT(Float::isReal(result));
    return result;
}

class Vector2 : public Vector<Vector2> {
public:

    static const int DIMENSION = 2;

    // union allowing to access values both by index and by name.
    union {
        struct {

            float x;

            float y;
        };

		// An array with all 2 components, mapped to the same memory as x and y.
        float _dim[DIMENSION];
    };

    INLINE Vector2( const float x, const float y ) : x(x), y(y) { }

    INLINE Vector2() { }

	// Constructs vector with all 2 components equal.
    INLINE explicit Vector2( const float value ): x(value), y(value) { }

    INLINE float get(const int axis) const {
        UPBP_ASSERT(unsigned(axis) < unsigned(DIMENSION));
        return _dim[axis];
    }

    INLINE float& get(const int axis) {
        UPBP_ASSERT(unsigned(axis) < unsigned(DIMENSION));
        return _dim[axis];
    }
};

// Calculates cross product of 2 vectors, assuming they lie in the z=0 plane. The result is Vector with x and y coordinates zero, 
// and only z coordinate non-zero. The non-zero coordinate is returned.
INLINE float crossZ(const Vector2 a, const Vector2 b) {
    return a.x * b.y - a.y * b.x;
}

typedef Vector2 Vec2f;

class Vector4 : public Vector<Vector4> {
public:

    static const int DIMENSION = 4;

	// union allowing to access values both by index and by name.
    union {
        struct {

            float x;

            float y;

            float z;

            float w;
        };

		// An array with all 4 components, mapped to the same memory as x, y, z and w.
        float _dim[DIMENSION];
    };

    INLINE Vector4( const float x, const float y, const float z, const float w ) : x(x), y(y), z(z), w(w) { }

    INLINE Vector4() { }

    // Constructs vector with all 4 components equal.
    INLINE explicit Vector4( const float value ): x(value), y(value), z(value), w(value) { }

    INLINE float get(const int axis) const {
        UPBP_ASSERT(unsigned(axis) < unsigned(DIMENSION));
        return _dim[axis];
    }

    INLINE float& get(const int axis) {
        UPBP_ASSERT(unsigned(axis) < unsigned(DIMENSION));
        return _dim[axis];
    }
};

class Vector5 : public Vector<Vector5> {
public:

    static const int DIMENSION = 5;

    // union allowing to access values both by index and by name.
    union {
        struct {

            float x;

            float y;

            float z;

            float p;

            float q;
        };

        // An array with all 5 components, mapped to the same memory as x, y, z, p and q.
        float _dim[DIMENSION];
    };

    INLINE Vector5( const float x, const float y, const float z, const float p, const float q ) 
        : x(x), y(y), z(z), p(p), q(q) { }

    INLINE Vector5() { }

	// Constructs vector with all 5 components equal.
    INLINE explicit Vector5( const float val ) : x(val), y(val), z(val), p(val), q(val) { }

    static const Vector5 ZERO;

    INLINE float get(const int axis) const {
        UPBP_ASSERT(unsigned(axis) < unsigned(DIMENSION));
        return _dim[axis];
    }

    INLINE float& get(const int axis) {
        UPBP_ASSERT(unsigned(axis) < unsigned(DIMENSION));
        return _dim[axis];
    }
};

class Vector6 : public Vector<Vector6> {
public:

    static const int DIMENSION = 6;

    // union allowing to access values both by index and by name.
    union {
        struct {

            float x;

            float y;

            float z;

            float p;
            float q;
            float r;
        };

		// An array with all 6 components, mapped to the same memory as x, y, z, p, r and q.
        float _dim[DIMENSION];
    };

    INLINE Vector6( const float x, const float y, const float z, const float p, const float q, const float r ) :
        x(x), y(y), z(z), p(p), q(q), r(r) {}

    INLINE Vector6() { }

	// Constructs vector with all 6 components equal.
    INLINE explicit Vector6( const float val ) : x(val), y(val), z(val), p(val), q(val), r(val) { }

    static const Vector6 ZERO;

    INLINE float get(const int axis) const {
        UPBP_ASSERT(unsigned(axis) < unsigned(DIMENSION));
        return _dim[axis];
    }

    INLINE float& get(const int axis) {
        UPBP_ASSERT(unsigned(axis) < unsigned(DIMENSION));
        return _dim[axis];
    }
};

// Prints Pos into output stream.
INLINE std::ostream& operator<<( std::ostream& os, const Pos& vector ) {
    os << vector.x() << ' ' << vector.y() << ' ' << vector.z();
    return os;
}

// Prints Dir into output stream.
INLINE std::ostream& operator<<( std::ostream& os, const Dir& vector ) {
    os << vector.x() << ' ' << vector.y() << ' ' << vector.z();
    return os;
}

// Prints Vector4 into output stream.
INLINE std::ostream& operator<<( std::ostream& os, const Vector4 vector ) {
    os << vector.x << ' ' << vector.y << ' ' << vector.z << ' ' << vector.w;
    return os;
}

// Prints Vector5 into output stream.
INLINE std::ostream& operator<<( std::ostream& os, const Vector5 vector ) {
    os << vector.x << ' ' << vector.y << ' ' << vector.z << ' ' << vector.p << ' ' << vector.q;
    return os;
}

// Prints Vector6 into output stream.
INLINE std::ostream& operator<<( std::ostream& os, const Vector6 vect ) {
    os << vect.x << ' ' << vect.y << ' ' << vect.z << ' ' << vect.p << ' ' << vect.q << ' ' << vect.r;
    return os;
}

// Loads Dir from input stream. Required format is 3 floating point numbers without any delimiters other than whitespace.
INLINE std::istream& operator>>( std::istream& is, Dir& vector ) {
    is >> vector.x() >> vector.y() >> vector.z();
    return is;
}

// Loads Pos from input stream. Required format is 3 floating point numbers without any delimiters other than whitespace.
INLINE std::istream& operator>>( std::istream& is, Pos& vector ) {
    is >> vector.x() >> vector.y() >> vector.z();
    return is;
}

#endif //__VECTOR_HXX__