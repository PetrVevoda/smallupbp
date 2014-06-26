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

// wrapper for four 32-bit ints

#include "sseUtil.h"
#include "sseMask.h"


class sse4Ints {
public:
    __m128i data;		// public to allow outside tinkering, as necessary

    FF_INLINE sse4Ints() {}

    FF_INLINE sse4Ints(__m128i input)
        : data(input) {}

    FF_INLINE sse4Ints(__m128 input)
        : data(reint(input)) {}

    FF_INLINE sse4Ints(int i0, int i1, int i2, int i3)
        : data(_mm_set_epi32(i3, i2, i1, i0)) {}	// order is reversed

    FF_INLINE sse4Ints(int *ip) {
        data = _mm_load_si128((__m128i *)ip);
    }

    FF_INLINE int operator [](int index) const {
        return ((int *)&data)[index];
    }

    //--- STATIC GENERATORS ---//
    static FF_INLINE sse4Ints zeros() {
        return sse4Ints(_mm_setzero_si128());
    }

    static FF_INLINE sse4Ints expand(int i) {
        return sse4Ints(_mm_set1_epi32(i));
    }

    //--- CONVERT ---//
    static FF_INLINE sse4Ints cast(const sseMask &rhs) {
        return rhs.data;
    }

    //--- ARITHMETIC ---//
    FF_INLINE sse4Ints operator +(const sse4Ints &rhs) const {
        return _mm_add_epi32(data, rhs.data);
    }

    FF_INLINE sse4Ints operator -(const sse4Ints &rhs) const {
        return _mm_sub_epi32(data, rhs.data);
    }

    FF_INLINE sse4Ints operator -() const {
        return _mm_sub_epi32(sse4Ints::zeros().data, data);
    }

    //--- BITWISE ---//
    FF_INLINE sse4Ints operator &(const sse4Ints &rhs) const {
        return _mm_and_si128(data, rhs.data);
    }

    FF_INLINE sse4Ints operator |(const sse4Ints &rhs) const {
        return _mm_or_si128(data, rhs.data);
    }

    FF_INLINE sse4Ints operator ^(const sse4Ints &rhs) const {
        return _mm_xor_si128(data, rhs.data);
    }

    FF_INLINE sse4Ints operator ~() const {
        return operator ^(sseMask::on().data);
    }

    //--- SHIFTING ---//
    FF_INLINE sse4Ints operator <<(int i) const {
        return _mm_slli_epi32(data, i);
    }

    FF_INLINE sse4Ints operator >>(int i) const {
        return _mm_srli_epi32(data, i);
    }

    //--- ASSIGNMENT ---//
    FF_INLINE sse4Ints &operator +=(const sse4Ints &rhs) {
        operator =(operator +(rhs)); return *this;
    }

    FF_INLINE sse4Ints &operator -=(const sse4Ints &rhs) {
        operator =(operator -(rhs)); return *this;
    }

    FF_INLINE sse4Ints &operator &=(const sse4Ints &rhs) {
        operator =(operator &(rhs)); return *this;
    }

    FF_INLINE sse4Ints &operator |=(const sse4Ints &rhs) {
        operator =(operator |(rhs)); return *this;
    }

    FF_INLINE sse4Ints &operator ^=(const sse4Ints &rhs) {
        operator =(operator ^(rhs)); return *this;
    }

    FF_INLINE sse4Ints &operator <<=(int i) {
        operator =(operator <<(i)); return *this;
    }

    FF_INLINE sse4Ints &operator >>=(int i) {
        operator =(operator >>(i)); return *this;
    }

    //--- COMPARISON ---//
    FF_INLINE sseMask operator ==(const sse4Ints &rhs) const {
        return _mm_cmpeq_epi32(data, rhs.data);
    }

    FF_INLINE sseMask operator !=(const sse4Ints &rhs) const {
        return ~(operator ==(rhs));
    }

    FF_INLINE sseMask operator <(const sse4Ints &rhs) const {
        return _mm_cmplt_epi32(data, rhs.data);
    }

    FF_INLINE sseMask operator <=(const sse4Ints &rhs) const {
        return ~(operator >(rhs));
    }

    FF_INLINE sseMask operator >(const sse4Ints &rhs) const {
        return _mm_cmpgt_epi32(data, rhs.data);
    }

    FF_INLINE sseMask operator >=(const sse4Ints &rhs) const {
        return ~(operator <(rhs));
    }

    //--- SHUFFLE ---//
    template <int i0, int i1, int i2, int i3>
    FF_INLINE sse4Ints shuffle() const {
        return sseImpl::shuffle<i0, i1, i2, i3>(reint(data));
    }

    //--- REDUCTION ---//

    // adds the 4 components into a single int
    FF_INLINE int reduce_add() const {
        sse4Ints temp1 = operator +(shuffle<1, 0, 3, 2>());
        sse4Ints temp2 = temp1 + temp1.shuffle<2, 3, 0, 1>();
        return temp2[0];
    }


};

//--- STORE ---//
static FF_INLINE
    void store4(int *dst, const sse4Ints &src) {
        _mm_store_si128((__m128i *)dst, src.data);
}

//--- BLEND ---//
static FF_INLINE
    sse4Ints blend4(const sseMask &mask,
    const sse4Ints &arg_true,
    const sse4Ints &arg_false)
{
    return sseImpl::blend4(mask.data, arg_true.data, arg_false.data);
}

//--- MIN and MAX ---//
static FF_INLINE
    sse4Ints min4(const sse4Ints &a, const sse4Ints &b) {
        return blend4(a < b, a, b);		// [<] is faster than [<=]
}

static FF_INLINE
    sse4Ints max4(const sse4Ints &a, const sse4Ints &b) {
        return blend4(a > b, a, b);		// [>] is faster than [>=]
}

// end of sse4Ints.h
