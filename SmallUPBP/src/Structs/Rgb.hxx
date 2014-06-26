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

#ifndef __RGB_HXX__
#define __RGB_HXX__

#include <iostream>
#include <xmmintrin.h>
#include <smmintrin.h>

#include "Vector.hxx"

// 8-bit RGB values, as used for LDR image output/for screen display.
struct RgbBytes {
    uint8 r, g, b;
    INLINE RgbBytes(const uint8 r, const uint8 g, const uint8 b) : r(r), g(g), b(b) { }
    INLINE explicit RgbBytes(const uint8 gray) : r(gray), g(gray), b(gray) { }
    INLINE RgbBytes() { }

    static INLINE uint8 toByte(const float in) {
        return uint8(Utils::clamp(int(in*255), 0, 255));
    }
};

// Linear sRGB color space value. Uses SSE to accelerate computation, and can store one extra value (see Pos, Dir for details).
class Rgb {    
public:
    __m128 data;

    INLINE Rgb( const float r, const float g, const float b ) {
        this->r() = r;
        this->g() = g;
        this->b() = b;
    }

    INLINE Rgb() { }

    INLINE explicit Rgb(const __m128& _sse) : data(_sse) { }

    INLINE explicit Rgb( const float color ) : data(_mm_set1_ps(color)) {}

    INLINE explicit Rgb( const Dir& src ) : data(src.data) { }

    INLINE explicit Rgb( const Pos& src ) : data(src.data) { }


    INLINE float extraData() const {
        return Sse::get(data, 3);
    }

    INLINE float& extraData() {
        return Sse::getRef(data, 3);
    }

    INLINE void setAndPreserveExtra(const Rgb& other) {
        this->r() = other.r();
        this->g() = other.g();
        this->b() = other.b();
    }


    INLINE const float r() const {
        return (*this)[0];
    }
    INLINE float& r() {
        return (*this)[0];
    }
    INLINE const float g() const {
        return (*this)[1];
    }
    INLINE float& g() {
        return (*this)[1];
    }
    INLINE const float b() const {
        return (*this)[2];
    }
    INLINE float& b() {
        return (*this)[2];
    }

    INLINE const float operator[](const int index) const {
        UPBP_ASSERT(unsigned(index) < 3);
        return Sse::get(data, index);
    }

    INLINE float& operator[](const int index) {
        UPBP_ASSERT(unsigned(index) < 3);
        return Sse::getRef(data, index);
    }       

    INLINE Pos getPos() const {
        return Pos(data);
    }

    INLINE Rgb operator+(const Rgb& other) const {
        return Rgb(_mm_add_ps(data, other.data));
    }

    INLINE Rgb operator-(const Rgb& other) const {
        return Rgb(_mm_sub_ps(data, other.data));
    }
    
    INLINE Rgb operator-() const {
        return Rgb(_mm_xor_ps(data, _mm_castsi128_ps(_mm_set1_epi32(0x80000000))));
    }
    
    INLINE Rgb operator*(const Rgb& other) const {
        return Rgb(_mm_mul_ps(data, other.data));
    }
    
    INLINE Rgb operator/(const Rgb& other) const {
        UPBP_ASSERT(other.r() != 0.f && other.g() != 0.f && other.b() != 0.f);
        return Rgb(_mm_div_ps(data, other.data));
    }
    
    INLINE Rgb& operator+=(const Rgb& other) {
        this->data = _mm_add_ps(data, other.data);
        return *this;
    }
    
    INLINE Rgb& operator-=(const Rgb& other) {
        this->data = _mm_sub_ps(data, other.data);
        return *this;
    }
    
    INLINE Rgb& operator*=(const Rgb& other) {
        this->data = _mm_mul_ps(data, other.data);
        return *this;
    }
    
    INLINE Rgb& operator/=(const Rgb& other) {
        UPBP_ASSERT(other.r() != 0.f && other.g() != 0.f && other.b() != 0.f);
        this->data = _mm_div_ps(data, other.data);
        return *this;
    }
    
    INLINE Rgb operator*(const float factor) const {
        return Rgb(_mm_mul_ps(data, _mm_set1_ps(factor)));
    }
    
    INLINE Rgb operator/(const float factor) const {
        UPBP_ASSERT(factor != 0.f);
        return Rgb(_mm_div_ps(data, _mm_set1_ps(factor)));
    }
    
    INLINE Rgb& operator*=(const float factor) {
        this->data = _mm_mul_ps(data, _mm_set1_ps(factor));
        return *this;
    }
    
    INLINE Rgb& operator/=(const float factor) {
        UPBP_ASSERT(factor != 0.f);
        this->data = _mm_div_ps(data, _mm_set1_ps(factor));
        return *this;
    }
    
    friend INLINE Rgb operator*(const float factor, const Rgb& color) {
        return Rgb(_mm_mul_ps(color.data, _mm_set1_ps(factor)));
    }
    
    INLINE Rgb operator/(const int factor) const {
        UPBP_ASSERT(factor != 0);
        return Rgb(_mm_div_ps(data, _mm_set1_ps(float(factor))));
    }
    
    INLINE Rgb& operator/=(const int factor) {
        UPBP_ASSERT(factor != 0);
        this->data = _mm_div_ps(data, _mm_set1_ps(float(factor)));
        return *this;
    }

    // Returns 8-bit representation of this color.
    INLINE RgbBytes getBytes() const {
        const __m128i mapped = _mm_cvtps_epi32((getClamped()*255.f).data);
        const int* temp = (const int*) &mapped;
        return RgbBytes(uint8(temp[0]), uint8(temp[1]), uint8(temp[2]));
    }

    // Returns Same hue as this color, but with gray value of 1.
    INLINE Rgb getNormalized() const {
        return (*this) / this->grayValue();
    }

    // Tests if this Rgb is normalized. Returns true if grayValue of this Rgb is approximately 1.
    INLINE bool isNormalized() const {
		return abs(1.f - this->grayValue()) < 1e-4f;
    }

    // Returns true if all components are less than or equal to zero.
    INLINE bool isBlackOrNegative() const {
        return (_mm_movemask_ps(_mm_cmple_ps(data, _mm_set1_ps(0))) & 0x7) == 0x7;
    }

	// Returns true if all components are greater than zero.
	INLINE bool isPositive() const {
		return 
			Float::isPositive(r()) &&
			Float::isPositive(g()) &&
			Float::isPositive(b());
	}

	// Returns true if at least on of the components is NaN, infinity or negative.
	INLINE bool isNanInfNeg() const {
		return 
			Float::isNanInfNeg(r()) ||
			Float::isNanInfNeg(g()) ||
			Float::isNanInfNeg(b());
	}

    // Returns true if all components are greater than 1-eps.
    INLINE bool isWhite() const {
        return (_mm_movemask_ps(_mm_cmpge_ps(data, _mm_set1_ps(1.f - 1e-6f))) & 0x7) == 0x7;
    }

    // Returns clamped values of this color. That means that each component is clamped to fit range [0, 1].
    INLINE Rgb getClamped() const {
        return Rgb(_mm_min_ps(_mm_set1_ps(1.f), _mm_max_ps(_mm_set1_ps(0.f), data)));
    }

    INLINE float grayValue() const {
        return (r() + g() + b())/3.f;
        //return r*0.2989f + g*0.5870f + b*0.1140f;        
    }

    // Returns maximal component of this color.
    INLINE float max() const {
        return Utils::max(this->r(), this->g(), this->b());
    }

    // Returns minimal component of this color.
    INLINE float min() const {
        return Utils::min(r(), g(), b());
    }

    // Returns Rgb with absolute value of each component.
    INLINE Rgb absValues() const {
        return Rgb(_mm_and_ps(data, _mm_castsi128_ps(_mm_set1_epi32(0x7fFFffFF))));
    }

    // Returns Rgb with 1/values.
    INLINE Rgb invValues() const {
        return Rgb(_mm_div_ps(_mm_set1_ps(1.f), data));
    }

    // Component-wise maximum.
    static INLINE Rgb max(const Rgb& first, const Rgb& second) {
        return Rgb(_mm_max_ps(first.data, second.data));
    }

    // Component-wise minimum.
    static INLINE Rgb min(const Rgb& first, const Rgb& second) {
        return Rgb(_mm_min_ps(first.data, second.data));
    }

    // Tests if this Rgb represents meaningful color. Returns true if each component of this Rgb is non-negative and not too big or NAN.
    INLINE bool isValid() const {
        return (_mm_movemask_ps(_mm_and_ps(_mm_cmpeq_ps(data, data), _mm_and_ps(_mm_cmpge_ps(data, _mm_set1_ps(0.f)),
                _mm_cmple_ps(data, _mm_set1_ps(1e20f))))) & 0x7) == 0x7;
    }

    // Tests if all values of this color are real numbers. A real Rgb color can still be invalid (can have negative values), but not the other way around.
    INLINE bool isReal() const {
        return (_mm_movemask_ps(_mm_and_ps(_mm_and_ps(
            _mm_cmplt_ps(data, _mm_set1_ps(FLOAT_INFINITY)),
            _mm_cmpgt_ps(data, _mm_set1_ps(-FLOAT_INFINITY))            
            ),
            _mm_cmpeq_ps(data, data))) & 0x7) == 0x7;
    }
    
    INLINE bool operator==(const Rgb& second) const {
        return (_mm_movemask_ps(_mm_cmpeq_ps(data, second.data)) & 0x7) == 0x7;
    }
    
    INLINE bool operator!=(const Rgb& second) const {
        return !(second == * this);
    }

    // Approximate exponentiation - sufficient for gamma color mapping for display, but not stable with high exponents.
    INLINE Rgb powFast(const float exponent) const {
        return Rgb(Ff::powFast(this->data, exponent));
    }

    INLINE Rgb pow(const float exponent) const {
        return Rgb(::pow(r(), exponent), ::pow(g(), exponent), ::pow(b(), exponent));
    }

    INLINE float sum() const {
        return r() + g() + b();
    }

    // Clips this color so it lies in valid gamut.
    INLINE Rgb toGamut() const {
        return Rgb::max(Rgb::BLACK, *this);
    }

    INLINE Dir getDir() const {
        return Dir(this->data);
    }

    // Component-wise clamping. 
    static INLINE Rgb clamp(const Rgb& color, const Rgb& minimum, const Rgb& maximum) {
        return Rgb::min(Rgb::max(color, minimum), maximum);
    }

    // Returns simple approximation of the perceptual difference between two colors.
    static INLINE float perceptualDifference(const Rgb& x, const Rgb& y) {
        const float base = Utils::max(0.3f, x.grayValue()+y.grayValue())*0.5f;
        return (x-y).absValues().max() / base;
    }

    static INLINE Rgb exp(const Rgb& input) {
        return Rgb(Ff::exp(input.data));
    }

    INLINE Rgb log() const {
        return Rgb(Ff::log(this->data));
    }



    // White color (Rgb values 1, 1, 1)
    static const Rgb WHITE;

    // Black color (Rgb values 0, 0, 0)
    static const Rgb BLACK;

    // Red color (Rgb values 1, 0, 0)
    static const Rgb RED;

    // Green color (Rgb values 0, 1, 0)
    static const Rgb GREEN;

    // Blue color (Rgb values 0, 0, 1)
    static const Rgb BLUE;

    // Not a number components
    static const Rgb NAN;
};



// Loads color from input stream. Required format is 3 floats in a row with no delimiters other than whitespace.
INLINE std::istream& operator>>(std::istream& is, Rgb& color) {
    is >> color.r() >> color.g() >> color.b();
    return is;
}

// Prints color to output stream as 3 floats delimited by a whitespace.
INLINE std::ostream& operator<<(std::ostream& os, const Rgb& color) {
    os << color.r() << ' ' << color.g() << ' ' << color.b();
    return os;
}

#endif //__RGB_HXX__