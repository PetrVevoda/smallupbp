/*
The MIT License

Copyright (c) 2009 Peter Djeu, Michael Quinlan, and Peter Stone

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/


#pragma once
/*~API~*/

// wrapper for four 32-bit floats

#include "sseUtil.h"
#include "sseMask.h"


class sse4Floats {
public:
    __m128 data;		// public to allow outside tinkering, as necessary

    FF_INLINE sse4Floats() {}

    FF_INLINE sse4Floats(__m128 input)
        : data(input) {}

    FF_INLINE sse4Floats(__m128i input)
        : data(reint(input)) {}

    FF_INLINE sse4Floats(float f0, float f1, float f2, float f3)
        : data(_mm_set_ps(f3, f2, f1, f0)) {}	// order is reversed

    FF_INLINE sse4Floats(float *fp) {
        data = _mm_load_ps(fp);
    }

    FF_INLINE float operator [](int index) const {
        return ((float *)&data)[index];
    }

    //--- STATIC GENERATORS ---//
    static FF_INLINE sse4Floats zeros() {
        return sse4Floats(_mm_setzero_ps());
    }

    static FF_INLINE sse4Floats expand(float f) {
        return sse4Floats(_mm_set1_ps(f));
    }

    //--- ARITHMETIC ---//
    FF_INLINE sse4Floats operator +(const sse4Floats &rhs) const {
        return _mm_add_ps(data, rhs.data);
    }

    FF_INLINE sse4Floats operator -(const sse4Floats &rhs) const {
        return _mm_sub_ps(data, rhs.data);
    }

    FF_INLINE sse4Floats operator *(const sse4Floats &rhs) const {
        return _mm_mul_ps(data, rhs.data);
    }

    FF_INLINE sse4Floats operator /(const sse4Floats &rhs) const {
        return _mm_div_ps(data, rhs.data);
    }

    FF_INLINE sse4Floats operator -() const {
        return _mm_sub_ps(sse4Floats::zeros().data, data);
    }

    //--- BITWISE ---//
    FF_INLINE sse4Floats operator &(const sse4Floats &rhs) const {
        return _mm_and_ps(data, rhs.data);
    }

    FF_INLINE sse4Floats operator |(const sse4Floats &rhs) const {
        return _mm_or_ps(data, rhs.data);
    }

    FF_INLINE sse4Floats operator ^(const sse4Floats &rhs) const {
        return _mm_xor_ps(data, rhs.data);
    }

    FF_INLINE sse4Floats operator ~() const {
        return operator ^(sseMask::on().data);
    }

    //--- ASSIGNMENT ---//
    FF_INLINE sse4Floats &operator +=(const sse4Floats &rhs) {
        operator =(operator +(rhs)); return *this;
    }

    FF_INLINE sse4Floats &operator -=(const sse4Floats &rhs) {
        operator =(operator -(rhs)); return *this;
    }

    FF_INLINE sse4Floats &operator *=(const sse4Floats &rhs) {
        operator =(operator *(rhs)); return *this;
    }

    FF_INLINE sse4Floats &operator /=(const sse4Floats &rhs) {
        operator =(operator /(rhs)); return *this;
    }

    FF_INLINE sse4Floats &operator &=(const sse4Floats &rhs) {
        operator =(operator &(rhs)); return *this;
    }

    FF_INLINE sse4Floats &operator |=(const sse4Floats &rhs) {
        operator =(operator |(rhs)); return *this;
    }

    FF_INLINE sse4Floats &operator ^=(const sse4Floats &rhs) {
        operator =(operator ^(rhs)); return *this;
    }

    //--- COMPARISON ---//
    FF_INLINE sseMask operator ==(const sse4Floats &rhs) const {
        return _mm_cmpeq_ps(data, rhs.data);
    }

    FF_INLINE sseMask operator !=(const sse4Floats &rhs) const {
        return _mm_cmpneq_ps(data, rhs.data);
    }

    FF_INLINE sseMask operator <(const sse4Floats &rhs) const {
        return _mm_cmplt_ps(data, rhs.data);
    }

    FF_INLINE sseMask operator <=(const sse4Floats &rhs) const {
        return _mm_cmple_ps(data, rhs.data);
    }

    FF_INLINE sseMask operator >(const sse4Floats &rhs) const {
        return _mm_cmpgt_ps(data, rhs.data);
    }

    FF_INLINE sseMask operator >=(const sse4Floats &rhs) const {
        return _mm_cmpge_ps(data, rhs.data);
    }

    //--- SHUFFLE ---//
    template <int i0, int i1, int i2, int i3>
    FF_INLINE sse4Floats shuffle() const {
        return sseImpl::shuffle<i0, i1, i2, i3>(data);
    }

    //--- REDUCTION ---//

    // adds the 4 components into a single float
    FF_INLINE float reduce_add() const {
        sse4Floats temp1 = operator +(shuffle<1, 0, 3, 2>());
        sse4Floats temp2 = temp1 + temp1.shuffle<2, 3, 0, 1>();
        return temp2[0];
    }

    // multiplies the 4 components into a single float
    FF_INLINE float reduce_mult() const {
        sse4Floats temp1 = operator *(shuffle<1, 0, 3, 2>());
        sse4Floats temp2 = temp1 * temp1.shuffle<2, 3, 0, 1>();
        return temp2[0];
    }
};

//--- STORE ---//
static FF_INLINE
    void store4(float *dst, const sse4Floats &src) {
        _mm_store_ps(dst, src.data);
}

//--- BLEND ---//
static FF_INLINE
    sse4Floats blend4(const sseMask &mask,
    const sse4Floats &arg_true,
    const sse4Floats &arg_false)
{
    return sseImpl::blend4(mask.data, arg_true.data, arg_false.data);
}

//--- MIN and MAX ---//
static FF_INLINE
    sse4Floats min4(const sse4Floats &a, const sse4Floats &b) {
        return _mm_min_ps(a.data, b.data);
}

static FF_INLINE
    sse4Floats max4(const sse4Floats &a, const sse4Floats &b) {
        return _mm_max_ps(a.data, b.data);
}

//--- COMPARISON ---//
static FF_INLINE
    sseMask nanMask(const sse4Floats &input) {
        return input != input;
}

// inclusive range test on [lo, hi]
static FF_INLINE
    sseMask inRangeMask(const sse4Floats &input,
    const sse4Floats &lo,
    const sse4Floats &hi)
{
    return (input >= lo) & (input <= hi);
}

// exclusive range test on (lo, hi)
static FF_INLINE
    sseMask exRangeMask(const sse4Floats &input,
    const sse4Floats &lo,
    const sse4Floats &hi)
{
    return (input > lo) & (input < hi);
}

// end of sse4Floats.h
