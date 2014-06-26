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

#ifndef __BOUNDINGBOX_HXX__
#define __BOUNDINGBOX_HXX__

#include "..\Path\Ray.hxx"
#include "Vector.hxx"

// Axis aligned bounding box (AABB).
template<class TPoint>
class BoundingBox {
public:

    // Point with smallest coordinates in every direction.
    TPoint point1;        

    // Point with biggest coordinates in every direction.
    TPoint point2;

    // Constructs default empty AABB.
    INLINE BoundingBox() : point1(TPoint(FLOAT_INFINITY)), point2(TPoint(-FLOAT_INFINITY)) {}

    // Constructs AABB tightly enclosing 2 input AABBs. They do not need to be ordered in any way.
    INLINE BoundingBox( const BoundingBox& first, const BoundingBox& second ) :
        point1(TPoint::min(first.point1, second.point1)), point2(TPoint::max(first.point2, second.point2)) {}

	// Constructs AABB tightly enclosing 2 arbitrary points. They do not need to be ordered in any way.
    INLINE BoundingBox( const TPoint point1, const TPoint point2 ) : 
        point1(TPoint::min(point1, point2)), point2(TPoint::max(point1, point2)) {}

    // Adds another BoundingBox to the current one, creating one that overlaps them both.
    INLINE BoundingBox& operator+=( const BoundingBox& other ) {
        this->point1 = TPoint::min(point1, other.point1);
        this->point2 = TPoint::max(point2, other.point2);
        return *this;
    }

    // Adds another Vector to this box, enlarging this box as necessary.
    INLINE BoundingBox& operator+=( const TPoint point ) {
        this->point1 = TPoint::min(this->point1, point);
        this->point2 = TPoint::max(this->point2, point);
        return *this;
    }

    // Returns bounding box tightly enclosing this and second bounding box.
    INLINE BoundingBox operator+( const BoundingBox& other ) const {
        return BoundingBox(TPoint::min(point1, other.point1), TPoint::max(point2, other.point2));
    }
    
    // Returns bounding box tightly enclosing this one and an additional point.
    INLINE BoundingBox operator+(const TPoint other) const {
        return BoundingBox(TPoint::min(point1, other), TPoint::max(point2, other));
    }

    // Intersects the bounding box with a ray, returns true if there is an intersection, and stores the minimal and maximal ray parameter of the intersection.
    // invDir is precomputed inverse values of ray.direction, so they don't have to be calculated repeatedly for single ray.
    INLINE bool intersect(const Ray& ray, const typename TPoint::DirType invDir, float& intervalMin, float& intervalMax) const;

    // Tests whether a BoundingBox is entirely contained in this one.
    INLINE bool contains( const BoundingBox& other ) const {    // SSE version for 3D is below
        for(int i = 0; i < TPoint::DIMENSION; ++i) {
            if(point1[i] > other.point1[i] || point2[i] < other.point2[i]) {
                return false;
            }
        }
        return true;
    }
    
    // Tells if a given point is inside BoundingBox.
    INLINE bool contains(const TPoint point) const {    // SSE version for 3D is below
        for(int i = 0; i < TPoint::DIMENSION; ++i) {
            if(point[i] > point2[i] || point[i] < point1[i]) {
                return false;
            }
        }
        return true;
    }

    // Returns intersection of this bounding box with another. Result should be tested if it is empty before using.
    INLINE BoundingBox getIntersection( const BoundingBox& other ) const {
        BoundingBox3 result;
        result.point1 = TPoint::max(this->point1, other.point1);
        result.point2 = TPoint::min(this->point2, other.point2);
        return result;
    }

    // Returns vector with size of this BoundingBox in every direction.
    INLINE typename TPoint::DirType size() const {
        return TPoint::DirType(point2 - point1);
    }

    INLINE TPoint getCenter() const {
        return TPoint( (point2+point1) * 0.5f );
    }

    INLINE float getArea() const;

    INLINE float getVolume() const {
        const TPoint::DirType sizes = this->size();
        float result = sizes[0];
        for(int i = 1; i < TPoint::DIMENSION; ++i) {
            result *= sizes[i];
        }
        return result;
    }
    
    INLINE bool operator==(const BoundingBox& other) const {
        return this->point1 == other.point1 && this->point2 == other.point2;
    }

    // Sets this BoundingBox to be empty. (from infinity to -infinity). Such BoundingBox is only a dummy and has undefined certain operations, such as getVoume, getArea, etc.
    INLINE void setEmpty() {
        this->point1 = TPoint(FLOAT_INFINITY);
        this->point2 = TPoint(-FLOAT_INFINITY);        
    }

    INLINE bool isEmpty() const {        // SSE version for 3D is below
        for(int i = 0; i < TPoint::DIMENSION; ++i) {
            if(point1[i] > point2[i]) {
                return true;
            }
        }
        return false;
    }

    // Returns squared shortest distance from this BoundingBox to a point.
    INLINE float distanceSquared(const TPoint vect) const {
        return TPoint::DirType::max(TPoint::DirType::max(point1 - vect, vect - point2), TPoint::DirType::ZERO).square();
    }

    // Returns squared largest distance from this BoundingBox to a point.
    INLINE float distanceSquaredMax(const TPoint vect) const {
        return TPoint::DirType::max(vect - point1, point2 - vect).square();
    }

    // Returns copy of this bounding box enlarged in each dimension by a factor (both by addition and multiplication).
    INLINE BoundingBox getEpsilonEnlarged(const float factor) const {
        TPoint center = this->getCenter();
        TPoint::DirType size = this->size();
        size *= (1.f+factor)*0.5f;
        size += TPoint::DirType(factor);
        return BoundingBox(center-size, center+size);
    }

    // Returns smallest axis-aligned cube enclosing this bounding box.
    INLINE BoundingBox getCube() const {
        const TPoint center = this->getCenter();
        const float size = this->size().max()*0.5f;
        return BoundingBox(center-TPoint::DirType(size), center+TPoint::DirType(size));
    }

    INLINE bool isReal() const {
        return point1.isReal() && point2.isReal();
    }

    // Used for iterating corners of the bounding box.
    INLINE TPoint getNthCorner(const int n) const;
};


typedef BoundingBox<Vector2> BoundingBox2;
typedef BoundingBox<Pos> BoundingBox3;
typedef BoundingBox<Vector4> BoundingBox4;
typedef BoundingBox<Vector6> BoundingBox6;


INLINE bool BoundingBox3::isEmpty() const {
    return (_mm_movemask_ps(_mm_cmpgt_ps(point1.data, point2.data)) & 0x7) == 0x7;
}

INLINE bool BoundingBox3::contains( const BoundingBox3& other ) const {
    return (_mm_movemask_ps(_mm_and_ps(_mm_cmple_ps(point1.data, other.point1.data), 
                                       _mm_cmpge_ps(point2.data, other.point2.data)))& 0x7) == 0x7;

}

INLINE bool BoundingBox3::contains(const Pos point) const {
    return (_mm_movemask_ps(_mm_and_ps(_mm_cmple_ps(point1.data, point.data), _mm_cmpge_ps(point2.data, point.data)))& 0x7) == 0x7;
}

float BoundingBox3::getArea() const {
    const Dir sizes = size();
    return 2*(sizes.x()*sizes.y() + sizes.x()*sizes.z() + sizes.y()*sizes.z());
}

Pos BoundingBox3::getNthCorner(const int n) const {
    UPBP_ASSERT(unsigned(n) < 8u);
    return Pos((n&1) ? point1.x() : point2.x(), (n&2) ? point1.y() : point2.y(), (n&4) ? point1.z() : point2.z());
}

// Intersects the bounding box with a ray, returns true if there is an intersection, and stores the minimal and maximal ray parameter of the intersection.
// invDir is precomputed inverse values of ray.direction, so they don't have to be calculated repeatedly for single ray.
// According to http://www.flipcode.com/archives/SSE_RayBox_Intersection_Test.shtml
INLINE bool BoundingBox<Pos>::intersect(const Ray& ray, const Pos::DirType inverseDirection, float& intervalMin, float& intervalMax) const {

//#define loadps(mem)		_mm_load_ps((const float * const)(mem))
#define storess(ss,mem)		_mm_store_ss((float * const)(mem),(ss))
#define minss			_mm_min_ss
#define maxss			_mm_max_ss
#define minps			_mm_min_ps
#define maxps			_mm_max_ps
#define mulps			_mm_mul_ps
#define subps			_mm_sub_ps
#define rotatelps(ps)		_mm_shuffle_ps((ps),(ps), 0x39)	// a,b,c,d -> b,c,d,a
#define muxhps(low,high)	_mm_movehl_ps((low),(high))	// low{a,b,c,d}|high{e,f,g,h} = {c,d,g,h}



    // You may already have those values hanging around somewhere.
    const __m128
        plus_inf	= _mm_set1_ps(FLOAT_INFINITY),
        minus_inf	= _mm_set1_ps(-FLOAT_INFINITY);

    // Use a div if inverted directions aren't available.
    const __m128 l1 = _mm_mul_ps((point1 - ray.origin).data, inverseDirection.data);
    const __m128 l2 = _mm_mul_ps((point2 - ray.origin).data, inverseDirection.data);

    // The order we use for those min/max is vital to filter out
    // NaNs that happens when an inv_dir is +/- inf and
    // (box_min - pos) is 0. inf * 0 = NaN
    const __m128 filtered_l1a = minps(l1, plus_inf);
    const __m128 filtered_l2a = minps(l2, plus_inf);

    const __m128 filtered_l1b = maxps(l1, minus_inf);
    const __m128 filtered_l2b = maxps(l2, minus_inf);

    // Now that we're back on our feet, test those slabs.
    __m128 lmax = maxps(filtered_l1a, filtered_l2a);
    __m128 lmin = minps(filtered_l1b, filtered_l2b);

    // Unfold back. Try to hide the latency of the shufps & co.
    const __m128 lmax0 = rotatelps(lmax);
    const __m128 lmin0 = rotatelps(lmin);
    lmax = minss(lmax, lmax0);
    lmin = maxss(lmin, lmin0);

    const __m128 lmax1 = muxhps(lmax,lmax);
    const __m128 lmin1 = muxhps(lmin,lmin);
    lmax = minss(lmax, lmax1);
    lmin = maxss(lmin, lmin1);

    if(_mm_comige_ss(lmax,lmin)) {       
        storess(lmin, &intervalMin);
        storess(lmax, &intervalMax);
        return true;
    } else {
        return false;
    }
}

// Prints BoundingBox into output stream.
template<class TPoint>
INLINE std::ostream& operator<<(std::ostream& os, const BoundingBox<TPoint>& aabb){
    os << "AABB{" << aabb.point1 << ' ' << aabb.point2 << '}';
    return os;
}

#endif //__BOUNDINGBOX_HXX__