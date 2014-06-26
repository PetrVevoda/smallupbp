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

/*~API~*/
#pragma once
// wrapper for four 32-bit masks

#include "sseUtil.h"


class sseMask {
private:
    static const int ELT_OFF = 0x00000000;	// mask element off
    static const int ELT_ON  = 0xffffffff;	// mask element on

    static FF_INLINE int getElt(bool b) {
        return b ? ELT_ON : ELT_OFF;
    }


    static const int BITS_PER_MASK_ELT = 32;
    static const int BITS_PER_SI128_SHIFT = 8;
    static const int SI128_SHIFTS_PER_MASK_ELEMENT =
        BITS_PER_MASK_ELT / BITS_PER_SI128_SHIFT;

public:
    __m128 data;		// public to allow outside tinkering, as necessary

    FF_INLINE sseMask() {}

    FF_INLINE sseMask(__m128 input)
        : data(input) {}

    FF_INLINE sseMask(__m128i input)
        : data(reint(input)) {}

    FF_INLINE sseMask(bool b0, bool b1, bool b2, bool b3)
        : data(reint(_mm_set_epi32(getElt(b3),
        getElt(b2),
        getElt(b1),
        getElt(b0)))) {}		// order is reversed

    FF_INLINE bool operator [](int index) const {
        return ((int *)&data)[index] == ELT_ON;
    }

    static FF_INLINE sseMask off() {
        return _mm_setzero_ps();
    }

    static FF_INLINE sseMask on() {
        return off() == off();
    }

    //--- BITWISE ---//
    FF_INLINE sseMask operator &(const sseMask &rhs) const {
        return _mm_and_ps(data, rhs.data);
    }

    FF_INLINE sseMask operator |(const sseMask &rhs) const {
        return _mm_or_ps(data, rhs.data);
    }

    FF_INLINE sseMask operator ^(const sseMask &rhs) const {
        return _mm_xor_ps(data, rhs.data);
    }

    FF_INLINE sseMask operator ~() const {
        return operator ^(sseMask::on());
    }

    
    //--- ASSIGNMENT ---//
    FF_INLINE sseMask &operator &=(const sseMask &rhs) {
        operator =(operator &(rhs)); return *this;
    }

    FF_INLINE sseMask &operator |=(const sseMask &rhs) {
        operator =(operator |(rhs)); return *this;
    }

    FF_INLINE sseMask &operator ^=(const sseMask &rhs) {
        operator =(operator ^(rhs)); return *this;
    }

    //INLINE sseMask &operator <<=(int index) {
    //	operator =(operator <<(index)); return *this;
    //}

    //INLINE sseMask &operator >>=(int index) {
    //	operator =(operator >>(index)); return *this;
    //}

    //--- COMPARISON ---//
    FF_INLINE sseMask operator ==(const sseMask &rhs) const {
        return _mm_cmpeq_epi32(reint(data), reint(rhs.data));
    }

    FF_INLINE sseMask operator !=(const sseMask &rhs) const {
        return ~(operator ==(rhs));
    }

    //--- SHUFFLE ---//
    template <int i0, int i1, int i2, int i3>
    FF_INLINE sseMask shuffle() const {
        return sseImpl::shuffle<i0, i1, i2, i3>(data);
    }


};

static FF_INLINE
    bool all(const sseMask &mask) {
        return _mm_movemask_ps(mask.data) == 0xf;
}

static FF_INLINE
    bool none(const sseMask &mask) {
        return _mm_movemask_ps(mask.data) == 0x0;
}

static FF_INLINE
    bool any(const sseMask &mask) {
        return _mm_movemask_ps(mask.data) != 0x0;
}

// end of sseMask.h
