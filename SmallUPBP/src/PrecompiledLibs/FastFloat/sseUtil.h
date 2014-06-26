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

// four 32-bit elements per SSE primitive
static const int SSE_WIDTH = 4;


#pragma warning(push)
//#pragma warning(disable: 1684)	// conversion from pointer to same-sized integral type (potential portability problem)

static FF_INLINE bool is_align16(void *p) {
    size_t i = (size_t)p;
    return i % 16 == 0;
}

#pragma warning(pop)

// reinterpret the bits of val as 4 floats, all bits are unchanged
static FF_INLINE __m128 reint(__m128i val) {
    return _mm_castsi128_ps(val);
}

// reinterpret the bits of val as 4 ints, all bits are unchanged
static FF_INLINE __m128i reint(__m128 val) {
    return _mm_castps_si128(val);
}

namespace sseImpl {
    // perform the shuffle on data, the element at i0 in data will
    // appear as element0 in the return value, the element at i1 in
    // data will appear as element1 in the return value, etc.,
    // duplicate indices in [i0, i3] are allowed
    template <int i0, int i1, int i2, int i3>
    FF_INLINE __m128 shuffle(__m128 data) {
        return _mm_shuffle_ps(data, data, _MM_SHUFFLE(i3, i2, i1, i0));
    }

    // wherever the mask is set, selects the entry in arg_true,
    // wherever the mask is not set, selects the entry in arg_false
    static FF_INLINE
        __m128  blend4(__m128 mask, __m128  arg_true, __m128  arg_false) {
            return _mm_or_ps(_mm_and_ps(mask, arg_true),
                _mm_andnot_ps(mask, arg_false));
    }

    // wherever the mask is set, selects the entry in arg_true,
    // wherever the mask is not set, selects the entry in arg_false
    static FF_INLINE
        __m128i blend4(__m128 mask, __m128i arg_true, __m128i arg_false) {
            __m128i imask = reint(mask);
            return _mm_or_si128(_mm_and_si128(imask, arg_true),
                _mm_andnot_si128(imask, arg_false));
    }
}

// end of sseUtil.h
